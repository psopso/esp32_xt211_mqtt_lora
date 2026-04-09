import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.const import CONF_ID

esp32xt211mqttlora_ns = cg.esphome_ns.namespace('esp32xt211mqttlora')
MyComponent = esp32xt211mqttlora_ns.class_('MyComponent', cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(MyComponent),
})

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)