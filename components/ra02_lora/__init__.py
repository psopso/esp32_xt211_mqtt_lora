import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID, CONF_CS_PIN
from esphome.components import spi
from esphome import pins

# Definice C++ namespace a třídy
ra02_lora_ns = cg.esphome_ns.namespace('ra02_lora')
Ra02Lora = ra02_lora_ns.class_('Ra02Lora', cg.Component, spi.SPIDevice)

# Parametry pro YAML
CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(Ra02Lora),
    cv.Required(CONF_CS_PIN): pins.gpio_output_pin_schema,
    cv.Required("reset_pin"): pins.gpio_output_pin_schema,
    cv.Required("dio0_pin"): pins.gpio_output_pin_schema,
}).extend(cv.COMPONENT_SCHEMA).extend(spi.spi_device_schema(False))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await spi.register_spi_device(var, config) # Propojí CS pin a SPI bus
    
    cg.add(var.set_reset_pin(config["reset_pin"]))
    cg.add(var.set_dio0_pin(config["dio0_pin"]))
