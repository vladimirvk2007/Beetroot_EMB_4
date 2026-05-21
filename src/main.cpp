#include <Arduino.h>
#include <ESP32Encoder.h>
#include <PID_v1.h>

// -------------------- Призначення пінів --------------------
#define PWM_OUT_PIN 18
#define ENCODER_A_PIN 4
#define ENCODER_B_PIN 5
#define POT_ADC_PIN 1

// -------------------- ШІМ --------------------
#define PWM_FREQ_HZ 20000
#define PWM_CHANNEL 0
#define PWM_RES_BITS 10
#define PWM_MAX_DUTY ((1 << PWM_RES_BITS) - 1)

// -------------------- Енкодер (PCNT, квадратурний x4) --------------------
// Для EC11 у режимі x4 це 80 відліків/оберт.
#define ENCODER_CPR_X4 80

#define PCNT_HIGH_LIMIT 32767
#define PCNT_LOW_LIMIT -32768

// -------------------- ПІД-регулятор --------------------
#define PID_KP 1.8
#define PID_KI 0.25
#define PID_KD 0.05

// -------------------- Логіка керування та безпеки --------------------
#define ANGLE_MIN_DEG 0.0f
#define ANGLE_MAX_DEG 360.0f
#define POSITION_TOLERANCE_DEG 1.0f

#define ADC_MIN 0
#define ADC_MAX 4095
#define ADC_MAX_STOP_THRESHOLD 4040
#define ADC_REARM_THRESHOLD 120
#define ADC_CCW_STEP_THRESHOLD 8

#define CONTROL_PERIOD_MS 10U
#define STATUS_PRINT_MS 200U

static ESP32Encoder g_encoder;

// Обмежує значення в заданому діапазоні [min_v, max_v].
float clampf(float value, float min_v, float max_v) {
    if (value < min_v) {
        return min_v;
    }
    if (value > max_v) {
        return max_v;
    }
    return value;
}

// Перетворює сире значення АЦП потенціометра у кут завдання 0..360 градусів.
float map_adc_to_angle_deg(int adc_raw) {
    const float normalized = clampf(static_cast<float>(adc_raw), ADC_MIN, ADC_MAX) / static_cast<float>(ADC_MAX);
    return ANGLE_MIN_DEG + normalized * (ANGLE_MAX_DEG - ANGLE_MIN_DEG);
}

// Зчитує лічильник енкодера та обчислює поточний кут у градусах.
float read_encoder_angle_deg() {
    const int64_t pulse_count = g_encoder.getCount();

    const float angle = (static_cast<float>(pulse_count) * 360.0f) / static_cast<float>(ENCODER_CPR_X4);
    return clampf(angle, ANGLE_MIN_DEG, ANGLE_MAX_DEG);
}

// Встановлює скважність ШІМ для двигуна у нормованому діапазоні 0.0..1.0.
void set_motor_pwm(float duty_0_to_1) {
    const float clamped_duty = clampf(duty_0_to_1, 0.0f, 1.0f);
    const uint32_t duty = static_cast<uint32_t>(clamped_duty * static_cast<float>(PWM_MAX_DUTY));

    ledcWrite(PWM_CHANNEL, duty);
}

// Ініціалізує периферію: UART, АЦП, ШІМ та енкодер.
void setup() {
    Serial.begin(115200);
    delay(200);

    analogReadResolution(12);
    analogSetPinAttenuation(POT_ADC_PIN, ADC_11db);

    if (!ledcSetup(PWM_CHANNEL, PWM_FREQ_HZ, PWM_RES_BITS)) {
        Serial.println("LEDC setup failed");
        while (true) {
            delay(1000);
        }
    }
    ledcAttachPin(PWM_OUT_PIN, PWM_CHANNEL);

    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    g_encoder.attachFullQuad(ENCODER_A_PIN, ENCODER_B_PIN);
    g_encoder.clearCount();

    if (ENCODER_CPR_X4 <= 0) {
        Serial.println("Invalid ENCODER_CPR_X4 constant");
        while (true) {
            delay(1000);
        }
    }

    Serial.println("PID position control started");
    Serial.println("If pot reaches max or is turned CCW, regulation latches OFF.");
    Serial.println("To re-enable, return pot to minimum position.");
}

// Основний цикл керування: зчитування, перевірки безпеки та PID-регулювання.
void loop() {
    static bool s_initialized = false;
    static double s_pid_input = 0.0;
    static double s_pid_output = 0.0;
    static double s_pid_setpoint = 0.0;
    static PID s_pid(&s_pid_input, &s_pid_output, &s_pid_setpoint, PID_KP, PID_KI, PID_KD, DIRECT);
    static bool s_regulation_enabled = true;
    static int s_prev_adc = 0;
    static uint32_t s_last_control_ms = 0;
    static uint32_t s_last_print_ms = 0;

    if (!s_initialized) {
        s_prev_adc = analogRead(POT_ADC_PIN);
        s_last_control_ms = millis();
        s_last_print_ms = s_last_control_ms;
        s_pid.SetSampleTime(CONTROL_PERIOD_MS);
        s_pid.SetOutputLimits(0.0, 1.0);
        s_pid.SetMode(AUTOMATIC);
        s_initialized = true;
    }

    const uint32_t now_ms = millis();
    if ((now_ms - s_last_control_ms) < CONTROL_PERIOD_MS) {
        return;
    }
    s_last_control_ms = now_ms;

    const int adc_raw = analogRead(POT_ADC_PIN);
    const float measured_angle_deg = read_encoder_angle_deg();

    if (!s_regulation_enabled) {
        set_motor_pwm(0.0f);

        if (adc_raw <= ADC_REARM_THRESHOLD) {
            s_regulation_enabled = true;
            s_pid.SetMode(MANUAL);
            s_pid_output = 0.0;
            s_pid.SetMode(AUTOMATIC);
            Serial.println("Regulation re-enabled (pot at minimum)");
        }
    } else {
        const bool reached_max = adc_raw >= ADC_MAX_STOP_THRESHOLD;
        const bool turned_ccw = adc_raw < (s_prev_adc - ADC_CCW_STEP_THRESHOLD);

        if (reached_max || turned_ccw) {
            s_regulation_enabled = false;
            set_motor_pwm(0.0f);
            s_pid.SetMode(MANUAL);
            s_pid_output = 0.0;
            s_pid.SetMode(AUTOMATIC);

            if (reached_max) {
                Serial.println("Regulation latched OFF: pot at maximum");
            } else {
                Serial.println("Regulation latched OFF: pot turned CCW");
            }
        } else {
            const float setpoint_deg = map_adc_to_angle_deg(adc_raw);
            const float error_deg = setpoint_deg - measured_angle_deg;

            s_pid_setpoint = static_cast<double>(setpoint_deg);
            s_pid_input = static_cast<double>(measured_angle_deg);

            if (error_deg <= POSITION_TOLERANCE_DEG) {
                set_motor_pwm(0.0f);
            } else {
                s_pid.Compute();
                const float pwm_cmd = static_cast<float>(s_pid_output);
                set_motor_pwm(pwm_cmd);
            }

            if ((now_ms - s_last_print_ms) >= STATUS_PRINT_MS) {
                s_last_print_ms = now_ms;
                Serial.printf("ADC=%d SP=%.1fdeg PV=%.1fdeg ERR=%.1fdeg\n", adc_raw, setpoint_deg, measured_angle_deg,
                                            error_deg);
            }
        }
    }

    s_prev_adc = adc_raw;
}
