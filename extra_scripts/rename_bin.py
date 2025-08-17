Import("env")
# Rename firmware binary according to custom_prog_name in platformio.ini
project_name = env.GetProjectOption("custom_prog_name", "BaseApp")
env.Replace(PROGNAME=project_name)