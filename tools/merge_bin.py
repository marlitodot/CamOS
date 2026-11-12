# PlatformIO post-build script:
# erzeugt bin/CamOS-esp32cam-factory.bin (komplett, Adresse 0x0)
# und      bin/CamOS-esp32cam-ota.bin     (nur App, fuer OTA-Update / Adresse 0x10000)
Import("env")
import os, shutil, subprocess

def merge_bin(source, target, env):
    build_dir = env.subst("$BUILD_DIR")
    app = os.path.join(build_dir, env.subst("${PROGNAME}.bin"))
    out_dir = os.path.join(env.subst("$PROJECT_DIR"), "bin")
    os.makedirs(out_dir, exist_ok=True)

    images = []
    for offset, path in env.get("FLASH_EXTRA_IMAGES", []):
        images += [env.subst(offset), env.subst(path)]
    images += [env.subst(env.get("ESP32_APP_OFFSET", "0x10000")), app]

    factory = os.path.join(out_dir, "CamOS-esp32cam-factory.bin")
    esptool = os.path.join(env.PioPlatform().get_package_dir("tool-esptoolpy") or "", "esptool.py")
    cmd = [env.subst("$PYTHONEXE"), esptool, "--chip", "esp32", "merge_bin",
           "-o", factory, "--flash_mode", "dio", "--flash_freq", "40m",
           "--flash_size", "4MB"] + images
    print("Merging:", " ".join(cmd))
    subprocess.check_call(cmd)
    shutil.copyfile(app, os.path.join(out_dir, "CamOS-esp32cam-ota.bin"))
    print("==> Fertig:", factory)

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", merge_bin)
