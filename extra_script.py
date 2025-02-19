Import("env")

if env.get("PIOPLATFORM") == "XXXespressif32":
    env.Replace(SRC_FILTER=["+<*>", "-<*_spi*>", "+<*_esp_spi.c>"])
else:
    env.Replace(SRC_FILTER=["+<*>", "-<*_spi*>"])

