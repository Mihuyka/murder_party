#!/usr/bin/env python
import os
import sys
import subprocess

from methods import print_error

libname = "EXTENSION-NAME"
projectdir = "project"

localEnv = Environment(tools=["default"], PLATFORM="")

# Build profiles can be used to decrease compile times.
# You can either specify "disabled_classes", OR
# explicitly specify "enabled_classes" which disables all other classes.
# Modify the example file as needed and uncomment the line below or
# manually specify the build_profile parameter when running SCons.

# localEnv["build_profile"] = "build_profile.json"

customs = ["custom.py"]
customs = [os.path.abspath(path) for path in customs]

opts = Variables(customs, ARGUMENTS)
opts.Update(localEnv)

Help(opts.GenerateHelpText(localEnv))

env = localEnv.Clone()

if not (os.path.isdir("godot-cpp") and os.listdir("godot-cpp")):
    print_error("""godot-cpp is not available within this folder, as Git submodules haven't been initialized.
Run the following command to download godot-cpp:

    git submodule update --init --recursive""")
    sys.exit(1)

env = SConscript("godot-cpp/SConstruct", {"env": env, "customs": customs})

# ================== Добавленная поддержка gRPC/Protobuf ==================

# Проверяем наличие protoc и grpc_cpp_plugin
def find_program(name):
    for path in os.environ["PATH"].split(os.pathsep):
        full = os.path.join(path, name)
        if os.path.exists(full) and os.access(full, os.X_OK):
            return full
    return None

protoc = find_program("protoc")
grpc_plugin = find_program("grpc_cpp_plugin")

if not protoc:
    print_error("protoc not found. Please install protobuf compiler.")
    sys.exit(1)
if not grpc_plugin:
    print_error("grpc_cpp_plugin not found. Please install gRPC C++ plugin.")
    sys.exit(1)

# Определяем пути для генерации
proto_file = "proto/murder_trivia.proto"
proto_gen_dir = "src/gen/proto"   # куда будем генерировать

# Создаём директорию для сгенерированных файлов (если её нет)
if not os.path.exists(proto_gen_dir):
    os.makedirs(proto_gen_dir)

# Имена файлов без расширения
proto_name = "murder_trivia"
pb_cc = os.path.join(proto_gen_dir, proto_name + ".pb.cc")
pb_h = os.path.join(proto_gen_dir, proto_name + ".pb.h")
grpc_cc = os.path.join(proto_gen_dir, proto_name + ".grpc.pb.cc")
grpc_h = os.path.join(proto_gen_dir, proto_name + ".grpc.pb.h")

# Команда для генерации protobuf и gRPC кода
gen_cmd = (
    f"{protoc} --proto_path=proto "
    f"--cpp_out={proto_gen_dir} "
    f"--grpc_out={proto_gen_dir} "
    f"--plugin=protoc-gen-grpc={grpc_plugin} "
    f"{proto_file}"
)

# Добавляем команду в SCons
env.Command(
    target=[pb_cc, pb_h, grpc_cc, grpc_h],
    source=proto_file,
    action=gen_cmd,
    )  # В SCons можно указать несколько target в списке

# Добавляем include пути для сгенерированных заголовков и системные пути
# Для Linux используем pkg-config, для других ОС можно дописать аналоги
if env["platform"] == "linux":
    # Получаем флаги через pkg-config
    try:
        pkg_cflags = subprocess.check_output(["pkg-config", "--cflags", "grpc++", "protobuf"], text=True).strip()
        pkg_libs = subprocess.check_output(["pkg-config", "--libs", "grpc++", "protobuf"], text=True).strip()
    except subprocess.CalledProcessError:
        print_error("pkg-config failed. Please ensure grpc++ and protobuf are installed with pkg-config support.")
        sys.exit(1)
    # Добавляем флаги компиляции и линковки
    env.Append(CCFLAGS=pkg_cflags.split())
    env.Append(LINKFLAGS=pkg_libs.split())

# Добавляем путь к сгенерированным заголовкам
env.Append(CPPPATH=[proto_gen_dir, "src"])

# Добавляем сгенерированные исходники в общий список
sources = Glob("src/*.cpp")
sources.extend([pb_cc, grpc_cc])

# ================== Конец добавленной части ==================

if env["target"] in ["editor", "template_debug"]:
    try:
        doc_data = env.GodotCPPDocData("src/gen/doc_data.gen.cpp", source=Glob("doc_classes/*.xml"))
        sources.append(doc_data)
    except AttributeError:
        print("Not including class reference as we're targeting a pre-4.3 baseline.")

# .dev doesn't inhibit compatibility, so we don't need to key it.
# .universal just means "compatible with all relevant arches" so we don't need to key it.
suffix = env['suffix'].replace(".dev", "").replace(".universal", "")

lib_filename = "{}{}{}{}".format(env.subst('$SHLIBPREFIX'), libname, suffix, env.subst('$SHLIBSUFFIX'))

library = env.SharedLibrary(
    "bin/{}/{}".format(env['platform'], lib_filename),
    source=sources,
)

copy = env.Install("{}/bin/{}/".format(projectdir, env["platform"]), library)

# (Опционально) Добавляем RPATH для библиотек gRPC/Protobuf
if env["platform"] == "linux":
    # Находим директорию libgrpc++ через pkg-config
    try:
        lib_dir = subprocess.check_output(["pkg-config", "--variable=libdir", "grpc++"], text=True).strip()
        # Добавляем RPATH в библиотеку (чтобы она искала зависимости в этом каталоге и $ORIGIN)
        env.Append(LINKFLAGS=[f"-Wl,-rpath,{lib_dir}", "-Wl,-rpath,$ORIGIN"])
    except:
        pass  # если не удалось, просто пропускаем

default_args = [library, copy]
Default(*default_args)