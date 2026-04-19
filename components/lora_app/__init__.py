# lora_app/__init__.py
import esphome.codegen as cg
# ... (standardní importy) ...

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # ESPHome najde jakoukoliv komponentu podle ID z YAML
    hw_driver = await cg.get_variable(config[CONF_LORA_ID])
    
    # Zde probíhá samotný "Bind". Předáme instanci hardware do aplikace.
    cg.add(var.set_lora_driver(hw_driver))