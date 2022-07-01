#build_android_aot, engine/engine

import os
import os.path as path
import zipfile
import shutil
import glob

DART_BIN = path.join(os.getcwd(), "third_party", "dart",
                     "tools", "sdks", "dart-sdk", "bin")
ANDROID_HOME = path.join(os.getcwd(), "third_party", "android_tools", "sdk")
ICU_DATA_PATH = path.join(os.getcwd(), "third_party",
                          "icu", "flutter", "icudtl.dat")


def make_path(*components):
  return path.join(os.getcwd(), *components)


def execute_command(command):
  print(f"Executing command: '{command}'")
  exit_code = os.system(command)
  print(f"Command '{command}' executed with code {exit_code}")
  if exit_code != 0:
    raise SystemExit(f"Command {command} exited with code {exit_code}.")


def build(target):
  execute_command(f"ninja -C out/{target}")


def gn(params):
  execute_command("./flutter/tools/gn " + " ".join(params))


def check_cwd():
  cwd = os.getcwd()
  if not cwd.endswith("engine/src"):
    raise SystemExit("The script must run in the engine/src directory.")
  print("Script is running in engine/src")


def set_use_prebuild_dart_sdk(v):
  os.environ["FLUTTER_PREBUILT_DART_SDK"] = str(v)


def build_host():
  # host_debug
  print("Generating host_debug")
  gn([
      "--runtime-mode",
      "debug",
      "--no-lto",
      "--full-dart-sdk",
      "--prebuilt-dart-sdk",
      "--build-embedder-examples"
  ])
  # host_profile
  print("Generating host_profile")
  gn([
      "--runtime-mode",
      "profile",
      "--no-lto",
      "--prebuilt-dart-sdk",
      "--build-embedder-examples"
  ])
  # host_release
  print("Generating host_release")
  gn([
      "--runtime-mode",
      "release",
      "--no-lto",
      "--prebuilt-dart-sdk",
      "--build-embedder-examples"
  ])
  # build host_debug
  build("host_debug")
  # build host_profile
  build("host_profile")
  # build host_release
  build("host_release")
  host_debug_directory = path.join(os.getcwd(), "out", "host_debug")
  flutter_embedder_framework = path.join(
      host_debug_directory, "FlutterEmbedder.framework")
  shutil.make_archive(base_name=flutter_embedder_framework,
                      format="zip", root_dir=flutter_embedder_framework)

  host_debug_dills = make_path(
      "out", "host_debug", "flutter_patched_sdk", "*.dill.S")
  host_release_dills = make_path(
      "out", "host_release", "flutter_patched_sdk_product", "*.dill.S")

  for file_path_to_remove in glob.glob(host_debug_dills):
    os.remove(file_path_to_remove)

  shutil.make_archive(make_path("out", "flutter_patched_sdk"), format="zip",
                      root_dir=make_path("out", "host_debug"), base_dir="flutter_patched_sdk")

  flutter_patched_sdk_product = make_path(
      "out", "host_release", "flutter_patched_sdk_product")
  if path.exists(flutter_patched_sdk_product):
    shutil.rmtree(flutter_patched_sdk_product)
  os.rename(make_path("out", "host_release", "flutter_patched_sdk"),
            flutter_patched_sdk_product)

  for file_path_to_remove in glob.glob(host_release_dills):
    os.remove(file_path_to_remove)

  shutil.make_archive(make_path("out", "flutter_patched_sdk_product"), format="zip",
                      root_dir=make_path("out", "host_release"), base_dir="flutter_patched_sdk_product")


def build_mac():
  # mac_debug_arm64
  print("Generating mac_debug_arm64")
  gn([
      "--mac",
      "--mac-cpu",
      "arm64",
      "--runtime-mode",
      "debug",
      "--no-lto",
      "--full-dart-sdk",
      "--prebuilt-dart-sdk"
  ])
  # mac_profile_arm64
  print("Generating mac_profile_arm64")
  gn([
      "--mac",
      "--mac-cpu",
      "arm64",
      "--runtime-mode",
      "profile",
      "--no-lto",
      "--prebuilt-dart-sdk"
  ])
  # mac_release_arm64
  print("Generating mac_release_arm64")
  gn([
      "--mac",
      "--mac-cpu",
      "arm64",
      "--runtime-mode",
      "release",
      "--no-lto",
      "--prebuilt-dart-sdk"
  ])
  # build host_debug
  build("mac_debug_arm64")
  # build host_profile
  build("mac_profile_arm64")
  # build host_release
  build("mac_release_arm64")
  # debug macos
  package_macos_variant(
      "debug",
      "mac_debug_arm64",
      "host_debug",
      "darwin-x64"
  )
  # profile macos
  package_macos_variant(
      "profile",
      "mac_profile_arm64",
      "host_profile",
      "darwin-x64-profile"
  )
  # release macos
  package_macos_variant(
      "release",
      "mac_release_arm64",
      "host_release",
      "darwin-x64-release"
  )
  print("Creating darwin-x64 artifacts.zip")
  artifacts_debug_zip = path.join(
      os.getcwd(), "out", "darwin-x64", "artifacts.zip")
  host_debug_directory = path.join(os.getcwd(), "out", "host_debug")
  with zipfile.ZipFile(artifacts_debug_zip, "w") as zip_file:
    files = [
        (ICU_DATA_PATH, "icudtl.dat"),
        (path.join(host_debug_directory, "flutter_tester"), "flutter_tester"),
        (path.join(host_debug_directory, "impellerc"), "impellerc"),
        (path.join(host_debug_directory, "gen", "flutter", "lib",
         "snapshot", "isolate_snapshot.bin"), "isolate_snapshot.bin"),
        (path.join(host_debug_directory, "gen", "flutter", "lib",
         "snapshot", "vm_isolate_snapshot.bin"), "vm_isolate_snapshot.bin"),
        (path.join(host_debug_directory, "gen",
         "frontend_server.dart.snapshot"), "frontend_server.dart.snapshot"),
        (path.join(host_debug_directory, "gen_snapshot"), "gen_snapshot")
    ]
    for (file_path, file_name) in files:
      zip_file.write(file_path, arcname=file_name)
  print("Done creating darwin-x64 artifacts.zip")

  print("Creating darwin-x64-profile artifacts.zip")
  artifacts_profile_zip = path.join(
      os.getcwd(), "out", "darwin-x64-profile", "artifacts.zip")
  with zipfile.ZipFile(artifacts_profile_zip, "w") as zip_file:
    zip_file.write(path.join(os.getcwd(), "out", "host_profile",
                   "gen_snapshot"), arcname="gen_snapshot")
  print("Done creating darwin-x64-profile artifacts.zip")

  print("Creating darwin-x64-release artifacts.zip")
  artifacts_release_zip = path.join(
      os.getcwd(), "out", "darwin-x64-release", "artifacts.zip")
  with zipfile.ZipFile(artifacts_release_zip, "w") as zip_file:
    zip_file.write(path.join(os.getcwd(), "out", "host_release",
                   "gen_snapshot"), arcname="gen_snapshot")
  print("Done creating darwin-x64-release artifacts.zip")

  print("Creating darwin-x64 FlutterEmbedder.framework.zip")
  shutil.copy(make_path("out", "host_debug", "FlutterEmbedder.framework.zip"),
              make_path("out", "darwin-x64", "FlutterEmbedder.framework.zip"))
  print("Done creating darwin-x64 FlutterEmbedder.framework.zip")

  print("Creating darwin-x64 font-subset.zip")
  shutil.copy(make_path("out", "host_release", "zip_archives", "darwin-x64",
              "font-subset.zip"), make_path("out", "darwin-x64", "font-subset.zip"))
  print("Done creating darwin-x64 font-subset.zip")

  print("Creating dart-sdk-darwin-x64.zip")
  dart_sdk_x64_debug = make_path("out", "host_debug")
  dart_sdk_x64 = make_path("out", "dart-sdk-darwin-x64")
  shutil.make_archive(dart_sdk_x64, format="zip",
                      root_dir=dart_sdk_x64_debug, base_dir="dart-sdk")
  print("Done creating dart-sdk-darwin-x64.zip")

  print("Creating dart-sdk-darwin-arm64.zip")
  dart_sdk_arm64_debug = make_path("out", "mac_debug_arm64")
  dart_sdk_arm64 = make_path("out", "dart-sdk-darwin-arm64")
  shutil.make_archive(dart_sdk_arm64, format="zip",
                      root_dir=dart_sdk_arm64_debug, base_dir="dart-sdk")
  print("Done creating dart-sdk-darwin-arm64.zip")

  print("Creating flutter-web-sdk-darwin-x64.zip")
  dart_sdk_web_debug = make_path("out", "host_debug")
  dart_sdk_web = make_path("out", "flutter-web-sdk-darwin-x64")
  shutil.make_archive(dart_sdk_web, format="zip",
                      root_dir=dart_sdk_web_debug, base_dir="flutter_web_sdk")
  print("Done creating flutter-web-sdk-darwin-x64.zip")


def package_macos_variant(label, arm64_out, x64_out, bucket_name):
  out_directory = path.join(os.getcwd(), "out")
  label_directory = path.join(out_directory, label)
  arm64_directory = path.join(out_directory, arm64_out)
  x64_directory = path.join(out_directory, x64_out)
  bucket_directory = path.join(out_directory, bucket_name)
  create_macos_framework_command = f"""python\
  ./flutter/sky/tools/create_macos_framework.py\
  --dst {label_directory}\
  --arm64-out-dir {arm64_directory}\
  --x64-out-dir {x64_directory}"""
  if label == "release":
    create_macos_framework_command += " --dsym --strip"
  print(f"Create macOS {label} FlutterMacOS.framework")
  execute_command(create_macos_framework_command)
  create_macos_gen_snapshot_command = f"""python\
  ./flutter/sky/tools/create_macos_gen_snapshots.py\
  --dst {label_directory}\
  --arm64-out-dir {arm64_directory}\
  --x64-out-dir {x64_directory}"""
  print(f"Create macOS {label} gen_snapshot")
  execute_command(create_macos_gen_snapshot_command)
  framework = path.join(label_directory, "FlutterMacOS.framework")
  framework_zip = framework + ".zip"
  framework_zip_temp = framework + "_temp.zip"
  execute_command(f"zip -y -9  {framework_zip} {framework}")
  flutter_podspec = path.join(
      os.getcwd(), 'flutter/shell/platform/darwin/macos/framework/FlutterMacOS.podspec')
  with zipfile.ZipFile(framework_zip_temp, "w") as zip_file:
    zip_file.write(framework_zip, arcname="FlutterMacOS.framework.zip")
    zip_file.write(flutter_podspec, arcname="FlutterMacOS.podspec")
  os.remove(framework_zip)
  os.rename(framework_zip_temp, framework_zip)
  gen_snapshot_zip = path.join(label_directory, "gen_snapshot.zip")
  gen_snapshot_arm64 = path.join(label_directory, "gen_snapshot_arm64")
  gen_snapshot_x64 = path.join(label_directory, "gen_snapshot_x64")
  with zipfile.ZipFile(gen_snapshot_zip, "w") as zip_file:
    zip_file.write(gen_snapshot_arm64, arcname="gen_snapshot_arm64")
    zip_file.write(gen_snapshot_x64, arcname="gen_snapshot_x64")
  if path.isdir(bucket_directory):
    shutil.rmtree(bucket_directory)
  os.rename(label_directory, bucket_directory)


def get_maven_remote_name(artifact_filename):
  engine_hash = "92bf8421728f2aa0a896c88565094dd8e823d0da"
  artifact_id, artifact_extension = artifact_filename.split(".", 2)
  if artifact_id.endswith("-sources"):
    filename_pattern = '%s-1.0.0-%s-sources.%s'
  else:
    filename_pattern = '%s-1.0.0-%s.%s'

  artifact_id = artifact_id.replace('-sources', '')
  filename = filename_pattern % (
      artifact_id, engine_hash, artifact_extension
  )

  return 'io/flutter/%s/1.0.0-%s/%s' % (artifact_id, engine_hash, filename)


def create_maven_artifacts(maven_artifacts):
  for artifact in maven_artifacts:
    file_name = path.basename(artifact)
    remote_name = get_maven_remote_name(file_name)
    maven_directory = make_path("out", path.dirname(remote_name))
    os.makedirs(maven_directory, exist_ok=True)
    shutil.copy(artifact, make_path("out", remote_name))


def build_android_jit():
  print("Generating android_jit_release_x86")
  gn([
      "--android",
      "--android-cpu=x86",
      "--runtime-mode=jit_release"
  ])
  build("android_jit_release_x86")
  print("Creating android-x86-jit-release artifacts.zip")
  with zipfile.ZipFile(make_path("out", "android-x86-jit-release", "artifacts"), "w") as zip_file:
    zip_file.write(make_path("out", "android_jit_release_x86",
                   "flutter.jar"), arcname="flutter.jar")
  print("Done creating android-x86-jit-release artifacts.zip")


def build_android_debug():
  variants = [
      ('arm', 'android_debug', 'android-arm', 'armeabi_v7a'),
      ('arm64', 'android_debug_arm64', 'android-arm64', 'arm64_v8a'),
      #('x86', 'android_debug_x86', 'android-x86', 'x86'), NOT SUPPORTED
      ('x64', 'android_debug_x64', 'android-x64', 'x86_64'),
  ]
  for android_cpu, out_directory, artifacts_directory, abi in variants:
    print(f"Generating {out_directory}")
    gn([
        "--android",
        f"--android-cpu={android_cpu}",
        "--no-lto"
    ])
    build(out_directory)
    os.makedirs(make_path("out", artifacts_directory), exist_ok=True)
    print(f"Creating {artifacts_directory} artifacts.zip")
    with zipfile.ZipFile(make_path("out", artifacts_directory, "artifacts.zip"), "w") as zip_file:
      zip_file.write(make_path("out", out_directory,
                               "flutter.jar"), arcname="flutter.jar")
    print(f"Done creating {artifacts_directory} artifacts.zip")
    print(f"Creating {artifacts_directory} symbols.zip")
    with zipfile.ZipFile(make_path("out", artifacts_directory, "symbols.zip"), "w") as zip_file:
      zip_file.write(make_path("out", out_directory,
                               "libflutter.so"), arcname="libflutter.so")
    print(f"Done creating {artifacts_directory} symbols.zip")
    print(f"Creating MAVEN artifacts for {out_directory}")
    create_maven_artifacts([
        make_path(f"out", out_directory, f"{abi}_debug.jar"),
        make_path(f"out", out_directory, f"{abi}_debug.pom")
    ])
    print(f"Done creating MAVEN artifacts for {out_directory}")

  print(f"Creating MAVEN artifacts for embedding")
  create_maven_artifacts([
      make_path(f"out", "android_debug", "flutter_embedding_debug.jar"),
      make_path(f"out", "android_debug", "flutter_embedding_debug.pom"),
      make_path(f"out", "android_debug",
                "flutter_embedding_debug-sources.jar"),
  ])
  print(f"Done creating MAVEN artifacts for embedding")
  print("Creating sky_engine.zip")
  execute_command(f"ninja -C out/android_debug :dist")
  shutil.make_archive(base_name=make_path("out", "sky_engine"), format="zip",
                      root_dir=make_path("out", "android_debug", "dist", "packages"))
  print("Done creating sky_engine.zip")
  shutil.copy(make_path("out", "android_debug", "zip_archives",
              "android-javadoc.zip"), make_path("out", "android-javadoc.zip"))


def build_android_aot():
  variants = [
      # android_cpu, out_directory, artifacts_directory, clang_directory, android_triple, abi
      ("arm64", "android_profile_arm64", "android-arm64-profile",
       "clang_x64", "aarch64-linux-android", "arm64_v8a", "profile"),
      ("arm64", "android_release_arm64", "android-arm64-release",
          "clang_x64", "aarch64-linux-android", "arm64_v8a", "release"),
      ("arm", "android_profile", "android-arm-profile",
          "clang_x64", "arm-linux-androidabi", "armeabi_v7a", "profile"),
      ("arm", "android_release", "android-arm-release",
          "clang_x64", "arm-linux-androidabi", "armeabi_v7a", "release"),
      ("x64", "android_profile_x64", "android-x64-profile",
          "clang_x64", "x86_64-linux-android", "x86_64", "profile"),
      ("x64", "android_release_x64", "android-x64-release",
          "clang_x64", "x86_64-linux-android", "x86_64", "release"),
  ]

  for android_cpu, out_directory, artifacts_directory, clang_directory, android_triple, abi, runtime_mode in variants:
    gn([
        "--android",
        "--runtime-mode",
        runtime_mode,
        "--android-cpu",
        android_cpu
    ])
    build(out_directory)
    os.makedirs(make_path("out", artifacts_directory), exist_ok=True)
    print(f"Creating {artifacts_directory} artifacts.zip")
    with zipfile.ZipFile(make_path("out", artifacts_directory, "artifacts.zip"), "w") as zip_file:
      zip_file.write(make_path("out", out_directory,
                               "flutter.jar"), arcname="flutter.jar")
    print(f"Done creating {artifacts_directory} artifacts.zip")
    print(f"Creating {artifacts_directory} symbols.zip")
    with zipfile.ZipFile(make_path("out", artifacts_directory, "symbols.zip"), "w") as zip_file:
      zip_file.write(make_path("out", out_directory,
                               "libflutter.so"), arcname="libflutter.so")
    print(f"Done creating {artifacts_directory} symbols.zip")
    print(f"Creating MAVEN artifacts for {out_directory}")
    create_maven_artifacts([
        make_path(f"out", out_directory, f"{abi}_{runtime_mode}.jar"),
        make_path(f"out", out_directory, f"{abi}_{runtime_mode}.pom")
    ])
    print(f"Done creating MAVEN artifacts for {out_directory}")
    with zipfile.ZipFile(make_path("out", artifacts_directory, "linux-x64.zip"), "w") as zip_file:
      zip_file.write(make_path("out", out_directory,
                     clang_directory, "gen_snapshot"), arcname="gen_snapshot")
    if out_directory == "android_profile_arm64" or out_directory == "android_release_arm64":
      print(f"Creating MAVEN artifacts for embedding")
      create_maven_artifacts([
          make_path(f"out", out_directory,
                    f"flutter_embedding_{runtime_mode}.jar"),
          make_path(f"out", out_directory,
                    f"flutter_embedding_{runtime_mode}.pom"),
          make_path(f"out", out_directory,
                    f"flutter_embedding_{runtime_mode}-sources.jar"),
      ])
    execute_command(f"ninja -C out/{out_directory} flutter/lib/snapshot")
    with zipfile.ZipFile(make_path("out", artifacts_directory, "darwin-x64.zip"), "w") as zip_file:
      zip_file.write(make_path("out", out_directory,
                     clang_directory, "gen_snapshot"), arcname="gen_snapshot")


def build_android():
  build_android_debug()
  build_android_aot()


def build_objc_doc():
  objc_doc_directory = make_path("out", "objcdoc")
  os.makedirs(objc_doc_directory, exist_ok=True)
  os.chdir("flutter")
  command = f"./tools/gen_objcdoc.sh {objc_doc_directory}"
  execute_command(command)
  os.chdir("..")
  shutil.make_archive(make_path("out", "ios-objcdoc"),
                      format="zip", root_dir=objc_doc_directory)


def package_ios_variant(
    label,
    arm64_out,
    armv7_out,
    sim_x64_out,
    sim_arm64_out,
    bucket_name,
    strip_bitcode=False
):
  out_directory = make_path("out")
  label_directory = make_path("out", label)
  create_ios_framework_command = " ".join([
      "./flutter/sky/tools/create_ios_framework.py",
      "--dst",
      label_directory,
      "--arm64-out-dir",
      path.join(out_directory, arm64_out),
      "--armv7-out-dir",
      path.join(out_directory, armv7_out),
      "--simulator-x64-out-dir",
      path.join(out_directory, sim_x64_out),
      "--simulator-arm64-out-dir",
      path.join(out_directory, sim_arm64_out),
  ])

  if strip_bitcode:
    create_ios_framework_command += " --strip-bitcode"

  if label == 'release':
    create_ios_framework_command += " --dsym --strip"

  execute_command(create_ios_framework_command)

  # Package the multi-arch gen_snapshot for macOS.
  create_macos_gen_snapshot_command = " ".join([
      "./flutter/sky/tools/create_macos_gen_snapshots.py",
      '--dst',
      label_directory,
      '--arm64-out-dir',
      path.join(out_directory, arm64_out),
      '--armv7-out-dir',
      path.join(out_directory, armv7_out),
  ])

  execute_command(create_macos_gen_snapshot_command)

  shutil.copy(make_path("flutter", "shell", "platform", "darwin", "ios",
              "framework", "Flutter.podspec"), make_path("out", label, "Flutter.podspec"))

  # Upload the artifacts to cloud storage.
  file_artifacts = [
      'Flutter.podspec',
      'gen_snapshot_armv7',
      'gen_snapshot_arm64',
  ]
  directory_artifacts = [
      'Flutter.xcframework',
  ]

  os.makedirs(path.join(label_directory, "artifacts"), exist_ok=True)
  shutil.copyfile(path.join(label_directory, file_artifacts[0]), path.join(
      label_directory, "artifacts", file_artifacts[0]))
  shutil.copyfile(path.join(label_directory, file_artifacts[1]), path.join(
      label_directory, "artifacts", file_artifacts[1]))
  shutil.copyfile(path.join(label_directory, file_artifacts[2]), path.join(
      label_directory, "artifacts", file_artifacts[2]))
  if path.exists(path.join(label_directory, "artifacts", "Flutter.xcframework")):
    shutil.rmtree(path.join(label_directory,
                  "artifacts", "Flutter.xcframework"))
  shutil.copytree(path.join(label_directory, directory_artifacts[0]), path.join(
      label_directory, "artifacts", "Flutter.xcframework"))
  shutil.make_archive(path.join(out_directory, bucket_name, "artifacts"), format="zip", root_dir=path.join(
      label_directory, "artifacts"))

  if label == 'release':
    dsym_zip = path.join(label_directory, 'Flutter.dSYM')
    shutil.make_archive(path.join(out_directory, bucket_name,
                        "Flutter.dSYM"), format="zip", root_dir=dsym_zip)


def build_ios():
  # ios_debug_sim
  gn([
      "--ios",
      "--runtime-mode",
      "debug",
      "--simulator",
      "--no-lto"
  ])
  # ios_debug_sim_arm64
  gn([
      "--ios",
      "--runtime-mode",
      "debug",
      "--simulator",
      "--simulator-cpu=arm64",
      "--no-lto",
      "--no-goma"
  ])
  # ios_debug
  gn([
      "--ios",
      "--runtime-mode",
      "debug",
      "--bitcode"
  ])
  # ios_debug_arm
  gn([
      "--ios",
      "--runtime-mode",
      "debug",
      "--bitcode",
      "--ios-cpu=arm"
  ])
  build("ios_debug_sim")
  build("ios_debug_sim_arm64")
  build("ios_debug")
  build("ios_debug_arm")
  build_objc_doc()
  package_ios_variant(
      "debug",
      "ios_debug",
      "ios_debug_arm",
      "ios_debug_sim",
      "ios_debug_sim_arm64",
      "ios"
  )
  gn([
      '--ios',
      '--runtime-mode',
      'profile',
      '--bitcode'
  ])
  gn([
      '--ios',
      '--runtime-mode',
      'profile',
      '--bitcode',
      '--ios-cpu=arm'
  ])
  build('ios_profile')
  build('ios_profile_arm')
  package_ios_variant(
      'profile',
      'ios_profile',
      'ios_profile_arm',
      'ios_debug_sim',
      'ios_debug_sim_arm64',
      'ios-profile'
  )
  gn([
      '--ios',
      '--runtime-mode',
      'release',
      '--bitcode',
      '--no-goma'
  ])
  gn([
      '--ios',
      '--runtime-mode',
      'release',
      '--bitcode',
      '--no-goma',
      '--ios-cpu=arm'
  ])
  build('ios_release')
  build('ios_release_arm')
  package_ios_variant(
      'release',
      'ios_release',
      'ios_release_arm',
      'ios_debug_sim',
      'ios_debug_sim_arm64',
      'ios-release'
  )
  package_ios_variant(
      'release',
      'ios_release',
      'ios_release_arm',
      'ios_debug_sim',
      'ios_debug_sim_arm64',
      'ios-release-nobitcode',
      True
  )


def main():
  check_cwd()
  set_use_prebuild_dart_sdk(True)
  build_host()
  build_mac()
  build_android()
  build_ios()


if __name__ == "__main__":
  main()
