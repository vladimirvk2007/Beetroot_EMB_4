Import("env")

HARD_FLOAT_FLAGS = ["-mfpu=fpv4-sp-d16", "-mfloat-abi=hard"]
SOFT_FLOAT_FLAG = "-mfloat-abi=soft"


def _remove_flag(container_name, flag):
    flags = env.get(container_name, [])
    if isinstance(flags, list):
        env[container_name] = [item for item in flags if item != flag]


for bucket in ("CCFLAGS", "CXXFLAGS", "ASFLAGS", "LINKFLAGS"):
    _remove_flag(bucket, SOFT_FLOAT_FLAG)

env.AppendUnique(CCFLAGS=HARD_FLOAT_FLAGS)
env.AppendUnique(CXXFLAGS=HARD_FLOAT_FLAGS)
env.AppendUnique(ASFLAGS=HARD_FLOAT_FLAGS)
env.AppendUnique(LINKFLAGS=HARD_FLOAT_FLAGS)
