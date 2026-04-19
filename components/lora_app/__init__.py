# lora_app/__init__.py
import esphome.codegen as cg
# ... (standardní importy) ...
# Definice C++ namespace a třídy
lora_app_ns = cg.esphome_ns.namespace('lora_app')
Lora_App = ra02_lora_ns.class_('LoraApp', cg.Component)

# Parametry pro YAML
CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(LoraApp),
}).extend(cv.COMPONENT_SCHEMA))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # ESPHome najde jakoukoliv komponentu podle ID z YAML
    hw_driver = await cg.get_variable(config[CONF_LORA_ID])
    
    # Zde probíhá samotný "Bind". Předáme instanci hardware do aplikace.
    cg.add(var.set_lora_driver(hw_driver))