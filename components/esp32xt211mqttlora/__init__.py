import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

esp32xt211mqttlora_ns = cg.esphome_ns.namespace('esp32xt211mqttlora')
MyComponent = esp32xt211mqttlora_ns.class_('MyComponent', cg.Component)

# 🔽 nové konstanty
CONF_ROLE = "role"

CONF_MOSI = "mosi"
CONF_MISO = "miso"
CONF_SCK = "sck"
CONF_NSS = "nss"
CONF_RST = "rst"
CONF_DIO0 = "dio0"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(MyComponent),
 
    cv.Required(CONF_ROLE): cv.one_of("tx", "rx"),
    cv.Required(CONF_MOSI): cv.int_,
    cv.Required(CONF_MISO): cv.int_,
    cv.Required(CONF_SCK): cv.int_,
    cv.Required(CONF_NSS): cv.int_,
    cv.Required(CONF_RST): cv.int_,
    cv.Required(CONF_DIO0): cv.int_,
})

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    # 🔽 předání do C++
    cg.add(var.set_role(config[CONF_ROLE]))

    cg.add(var.set_mosi(config[CONF_MOSI]))
    cg.add(var.set_miso(config[CONF_MISO]))
    cg.add(var.set_sck(config[CONF_SCK]))
    cg.add(var.set_nss(config[CONF_NSS]))
    cg.add(var.set_rst(config[CONF_RST]))
    cg.add(var.set_dio0(config[CONF_DIO0]))

    await cg.register_component(var, config)
