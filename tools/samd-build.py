from os.path import isdir, join

from SCons.Script import DefaultEnvironment

env = DefaultEnvironment()
platform = env.PioPlatform()
board = env.BoardConfig()

FRAMEWORK_DIR = platform.get_package_dir("A52B")
assert isdir(FRAMEWORK_DIR)

BUILD_CORE = 'samd21'

# USB flags
ARDUINO_USBDEFINES = [("ARDUINO", 10805)]
if "build.usb_product" in env.BoardConfig():
    ARDUINO_USBDEFINES += [
        ("USB_VID", board.get("build.hwids")[0][0]),
        ("USB_PID", board.get("build.hwids")[0][1]),
        ("USB_PRODUCT", '\\"%s\\"' %
         board.get("build.usb_product", "").replace('"', "")),
        ("USB_MANUFACTURER", '\\"%s\\"' %
         board.get("vendor", "").replace('"', ""))
    ]

env.Append(
    ASFLAGS=["-x", "assembler-with-cpp"],

    CFLAGS=[
        "-std=gnu11"
    ],

    CCFLAGS=[
        "-Os",  # optimize for size
        "-ffunction-sections",  # place each function in its own section
        "-fdata-sections",
        "-Wall",
        "-mthumb",
        "-nostdlib",
        "--param", "max-inline-insns-single=500",
        "-mcpu=%s" % board.get("build.cpu")
    ],

    CXXFLAGS=[
        "-fno-rtti",
        "-fno-exceptions",
        "-std=gnu++11",
        "-fno-threadsafe-statics"
    ],

    CPPDEFINES=[
        ("F_CPU", "$BOARD_F_CPU"),
        "USBCON",
        "ARDUINO_ARCH_SAMD"
    ],

    CPPPATH=[
        join(FRAMEWORK_DIR, "cores", BUILD_CORE),
        join(FRAMEWORK_DIR, "libraries", "ABCNeopixel")
    ],

    LINKFLAGS=[
        "-Os",
        "-mthumb",
        # "-Wl,--cref", # don't enable it, it prints Cross Reference Table
        "-Wl,--gc-sections",
        "-Wl,--check-sections",
        "-Wl,--unresolved-symbols=report-all",
        "-Wl,--warn-common",
        "-Wl,--warn-section-align",
        "-mcpu=%s" % board.get("build.cpu"),
        "--specs=nosys.specs",
        "--specs=nano.specs"
    ],

    LIBS=["m"]
)


variants_dir = join(FRAMEWORK_DIR, "variants", "briki_mbcwb_samd21")


CMSIS_DIR = platform.get_package_dir("framework-cmsis")
CMSIS_ATMEL_DIR = platform.get_package_dir("framework-cmsis-atmel")
assert isdir(CMSIS_DIR) and isdir(CMSIS_ATMEL_DIR)

env.Append(
    CPPPATH=[
        join(CMSIS_DIR, "CMSIS", "Include"),
        join(CMSIS_ATMEL_DIR, "CMSIS", "Device", "ATMEL")
    ],

    LIBPATH=[
        join(CMSIS_DIR, "CMSIS", "Lib", "GCC"),
        variants_dir
    ]
)

env.Append(
    LIBPATH=[
        join(variants_dir, "linker_scripts", "gcc")
    ])


env.Prepend(
    LIBS=["arm_cortexM0l_math"]
)


env.Append(
    ASFLAGS=env.get("CCFLAGS", [])[:],
    CPPDEFINES=ARDUINO_USBDEFINES
)

#
# Lookup for specific core's libraries
#

env.Append(
    LIBSOURCE_DIRS=[
        join(FRAMEWORK_DIR, "libraries")
    ]
)

#
# Target: Build Core Library
#

libs = []

if not board.get("build.ldscript", ""):
        env.Append(
            LIBPATH=[
                join(variants_dir, "linker_scripts", "gcc")
            ])
        env.Replace(LDSCRIPT_PATH=board.get("build.arduino.ldscript", ""))

if "build.variant" in env.BoardConfig():
    if board.get("build.variant") != 'briki_mbcwb_samd21':
	    # custom variant
        custom_variants_dir = "$PROJECT_DIR"
        env.Append(
            CPPDEFINES=["CUSTOM_VARIANT"],
            CPPPATH=[join(custom_variants_dir, board.get("build.variant")),
                     variants_dir
            ]
        )
        libs.append(env.BuildLibrary(
            join("$BUILD_DIR", "FrameworkArduinoVariant"),
            join(custom_variants_dir, board.get("build.variant"))
        ))
    else:
        env.Append(
            CPPPATH=[variants_dir]
        )
        libs.append(env.BuildLibrary(
            join("$BUILD_DIR", "FrameworkArduinoVariant"),
            join(variants_dir)
        ))

libs.append(env.BuildLibrary(
    join("$BUILD_DIR", "FrameworkArduino"),
    join(FRAMEWORK_DIR, "cores", BUILD_CORE)
))


env.Prepend(LIBS=libs)
