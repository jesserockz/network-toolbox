"""ICMP ping, exposed as a runtime action.

ESPHome has no ping component of its own. This one is a thin wrapper around
ESP-IDF's ``esp_ping`` rather than a vendored copy of it, so it stays in step
with whatever lwIP the framework ships.
"""

from esphome import automation
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_COUNT, CONF_ID, CONF_IP_ADDRESS
from esphome.core import ID
from esphome.types import ConfigType, TemplateArgsType

CODEOWNERS = ["@jesserockz"]
DEPENDENCIES = ["network"]
MULTI_CONF = True

CONF_ON_RESULT = "on_result"

ping_ns = cg.esphome_ns.namespace("ping")
Pinger = ping_ns.class_("Pinger", cg.Component)
PingAction = ping_ns.class_("PingAction", automation.Action, cg.Parented.template(Pinger))


def _consume_ping_socket(config: ConfigType) -> ConfigType:
    """Account for the raw ICMP socket esp_ping opens per session.

    esp_ping_new_session() calls socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP), which
    draws one descriptor from the same LWIP pool that CONFIG_LWIP_MAX_SOCKETS
    sizes. Only one batch runs per Pinger at a time, so one socket per instance.
    There is no RAW SocketType; the three buckets are summed for the pool, so UDP
    is just the least-wrong label.
    """
    from esphome.components import socket

    socket.consume_sockets(1, "ping", socket.SocketType.UDP)(config)
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Pinger),
            cv.Optional(CONF_ON_RESULT): automation.validate_automation({}),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    _consume_ping_socket,
)


async def to_code(config: ConfigType) -> None:
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    for conf in config.get(CONF_ON_RESULT, []):
        await automation.build_callback_automation(
            var,
            "add_on_result_callback",
            [(float, "loss"), (cg.uint32, "latency")],
            conf,
        )


@automation.register_action(
    "ping.ping",
    PingAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(Pinger),
            cv.Required(CONF_IP_ADDRESS): cv.templatable(cv.string),
            cv.Optional(CONF_COUNT, default=4): cv.templatable(
                cv.int_range(min=1, max=60)
            ),
        }
    ),
    # The batch takes seconds; the action resumes the automation when it ends.
    synchronous=False,
)
async def ping_action_to_code(
    config: ConfigType,
    action_id: ID,
    template_arg: cg.TemplateArguments,
    args: TemplateArgsType,
) -> cg.Pvariable:
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    cg.add(
        var.set_ip_address(
            await cg.templatable(config[CONF_IP_ADDRESS], args, cg.std_string)
        )
    )
    cg.add(var.set_count(await cg.templatable(config[CONF_COUNT], args, cg.uint32)))
    return var
