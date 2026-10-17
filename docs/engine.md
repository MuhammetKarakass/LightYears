# Vega engine dependency

LightYears consumes a sibling [Vega checkout](../../Vega), defaulting to `../Vega`.
The engine owns runtime, scripting, GAS (`sas`), Sky, editor core, templates and
engine tooling/documentation. See its [game consumption contract](../../Vega/docs/new-game.md).

Stage B was validated against Stage A commit `7eef946` in `D:/Vega-engine-next`.
Override the checkout with `-DVEGA_ENGINE_DIR=D:/Vega-engine-next` or the same
environment variable when using `tools/vega-build.cmd`. No engine sources are vendored here.
