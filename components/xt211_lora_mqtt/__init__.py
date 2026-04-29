# lora_app/__init__.py

import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID
# Tímto importem získáme přístup k definici driveru
from .. import ra02_lora_lib

CONF_LORA_ID = "lora_id"

# ... (standardní importy) ...
# Definice C++ namespace a třídy
lora_app_ns = cg.esphome_ns.namespace('lora_app')
lora_app = lora_app_ns.class_('LoRaMqttGateway', cg.Component)

# Parametry pro YAML
CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(lora_app),
    # Vyžadujeme ID existující komponenty ra02_lora
    cv.Required(CONF_LORA_ID): cv.use_id(ra02_lora_lib.Ra02Lora),
    cv.Optional("data_topic"): cv.string,
    cv.Optional("status_topic"): cv.string,
    cv.Optional("battery_topic"): cv.string,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # ESPHome najde jakoukoliv komponentu podle ID z YAML
    hw_driver = await cg.get_variable(config[CONF_LORA_ID])
    
    # Zde probíhá samotný "Bind". Předáme instanci hardware do aplikace.
    cg.add(var.set_lora_driver(hw_driver))
    cg.add(var.set_data_topic(config["data_topic"]))
    cg.add(var.set_status_topic(config["status_topic"]))
    cg.add(var.set_battery_topic(config["battery_topic"]))
