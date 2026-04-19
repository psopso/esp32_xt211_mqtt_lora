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
lora_app = lora_app_ns.class_('LoraApp', cg.Component)

# Parametry pro YAML
CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(lora_app),
    # Vyžadujeme ID existující komponenty ra02_lora
    cv.Required(CONF_LORA_ID): cv.use_id(ra02_lora.Ra02Lora),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # ESPHome najde jakoukoliv komponentu podle ID z YAML
    hw_driver = await cg.get_variable(config[CONF_LORA_ID])
    
    # Zde probíhá samotný "Bind". Předáme instanci hardware do aplikace.
    cg.add(var.set_lora_driver(hw_driver))