#include <Arduino.h>
#include <ESP32Encoder.h>
#include <PID_v1.h>

// -------------------- Призначення пінів --------------------
#define PWM_OUT_PIN 18
#define ENCODER_A_PIN 15
#define ENCODER_B_PIN 16
#define POT_ADC_PIN 4

// -------------------- ШІМ --------------------
#define PWM_FREQ_HZ 20000
#define PWM_CHANNEL 0
#define PWM_RES_BITS 10
#define PWM_MAX_DUTY ((1 << PWM_RES_BITS) - 1)

// -------------------- Енкодер (PCNT, квадратурний x4) --------------------
#define ENCODER_CPR_X4 80

#define PCNT_HIGH_LIMIT 32767
#define PCNT_LOW_LIMIT -32768

// -------------------- ПІД-регулятор --------------------
#define PID_KP 0.1
#define PID_KI 0.1
#define PID_KD 0.1

// -------------------- Логіка керування --------------------
#define ANGLE_MIN_DEG 0.0
#define ANGLE_MAX_DEG 360.0

#define ADC_MIN 0
#define ADC_MAX 4095
#define ADC_REARM_THRESHOLD 120

#define CONTROL_PERIOD_MS 20U
#define STATUS_PRINT_MS 200U

static ESP32Encoder g_encoder;

// Обмежує значення в заданому діапазоні [min_v, max_v].
double clamp_value(double value, double min_v, double max_v) {
    if (value < min_v) {
        return min_v;
    }
    if (value > max_v) {
        return max_v;
    }
    return value;
}

// Перетворює сире значення АЦП потенціометра у кут завдання 0..360 градусів.
double map_adc_to_angle_deg(int adc_raw) {
    const double normalized = clamp_value(adc_raw, ADC_MIN, ADC_MAX) / ADC_MAX;
    return ANGLE_MIN_DEG + normalized * (ANGLE_MAX_DEG - ANGLE_MIN_DEG);
}

// Зчитує лічильник енкодера та обчислює поточний кут у градусах.
double read_encoder_angle_deg() {
    const int64_t pulse_count = g_encoder.getCount();

    const double angle = (pulse_count * 360.0) / ENCODER_CPR_X4;
    return clamp_value(angle, ANGLE_MIN_DEG, ANGLE_MAX_DEG);
}

// Встановлює скважність ШІМ для двигуна у нормованому діапазоні 0.0..1.0.
void set_motor_pwm(double duty_0_to_1) {
    const double clamped_duty = clamp_value(duty_0_to_1, 0.0, 1.0);
    const uint32_t duty = clamped_duty * PWM_MAX_DUTY;

    ledcWrite(PWM_CHANNEL, duty);
}

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
    Serial.println("Setpoint holds the maximum observed ADC value.");
    Serial.println("Reset (clear angle/setpoint) only when pot is near minimum.");
}

void loop() {
    static bool initialized = false;
    static bool regulation_enabled = true;
    static double pid_input = 0.0;
    static double pid_output = 0.0;
    static double pid_setpoint = 0.0;
    static PID pid(&pid_input, &pid_output, &pid_setpoint, PID_KP, PID_KI, PID_KD, DIRECT);
    static int hold_adc = 0;
    static uint32_t last_control_ms = 0;
    static uint32_t last_print_ms = 0;

    if (!initialized) {
        hold_adc = analogRead(POT_ADC_PIN);
        last_control_ms = millis();
        last_print_ms = last_control_ms;
        pid.SetSampleTime(CONTROL_PERIOD_MS);
        pid.SetOutputLimits(0.0, 1.0);
        pid.SetMode(AUTOMATIC);
        initialized = true;
    }

    const uint32_t now_ms = millis();
    if ((now_ms - last_control_ms) < CONTROL_PERIOD_MS) {
        return;
    }
    last_control_ms = now_ms;

    const int adc_raw = analogRead(POT_ADC_PIN);
    const double measured_angle_deg = read_encoder_angle_deg();

    if (!regulation_enabled) {
        set_motor_pwm(0.0);

        if (adc_raw <= ADC_REARM_THRESHOLD) {
            g_encoder.clearCount();
            pid_input = 0.0;
            pid_setpoint = 0.0;
            hold_adc = adc_raw;
            pid.SetMode(MANUAL);
            pid_output = 0.0;
            pid.SetMode(AUTOMATIC);
            regulation_enabled = true;

            if (now_ms - last_print_ms >= STATUS_PRINT_MS) {
                last_print_ms = now_ms;
                Serial.printf("REARM: ADC=%d (<= %d), regulator enabled\n", adc_raw, ADC_REARM_THRESHOLD);
            }
        }
        return;
    }

    if (measured_angle_deg == ANGLE_MAX_DEG || adc_raw > ADC_MAX - ADC_REARM_THRESHOLD) {
        set_motor_pwm(0.0);
        pid.SetMode(MANUAL);
        pid_output = 0.0;
        regulation_enabled = false;

        if (now_ms - last_print_ms >= STATUS_PRINT_MS) {
            last_print_ms = now_ms;
            Serial.printf("LATCH OFF: ADC=%d PV=%.1fdeg\n", adc_raw, measured_angle_deg);
        }
        return;
    }

    if (adc_raw > hold_adc) {
        hold_adc = adc_raw;
    }

    const double setpoint_deg = map_adc_to_angle_deg(hold_adc);
    const double error_deg = setpoint_deg - measured_angle_deg;

    pid_setpoint = setpoint_deg;
    pid_input = measured_angle_deg;

    pid.Compute();
    set_motor_pwm(pid_output);

    if (now_ms - last_print_ms >= STATUS_PRINT_MS) {
        last_print_ms = now_ms;
        Serial.printf("ADC=%d HOLD=%d SP=%.1fdeg PV=%.1fdeg ERR=%.1fdeg PWM=%.2f\n", adc_raw, hold_adc,
                      setpoint_deg, measured_angle_deg, error_deg, pid_output);
    }

}
