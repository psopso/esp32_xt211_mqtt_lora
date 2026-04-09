import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

CONF_NSS = "nss"
CONF_RST = "rst"
CONF_DIO0 = "dio0"
CONF_ROLE = "role"

esp32xt211mqttlora_ns = cg.esphome_ns.namespace('esp32xt211mqttlora')
MyComponent = esp32xt211mqttlora_ns.class_('MyComponent', cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(MyComponent),

    cv.Required(CONF_NSS): cv.int_,
    cv.Required(CONF_RST): cv.int_,
    cv.Required(CONF_DIO0): cv.int_,
    cv.Optional(CONF_ROLE, default="rx"): cv.one_of("tx", "rx"),
})

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    cg.add(var.set_nss(config[CONF_NSS]))
    cg.add(var.set_rst(config[CONF_RST]))
    cg.add(var.set_dio0(config[CONF_DIO0]))
    cg.add(var.set_role(config[CONF_ROLE]))

    await cg.register_component(var, config)
