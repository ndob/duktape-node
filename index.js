var bindings = require("bindings")("duktape.node");

module.exports.run = bindings.run;
module.exports.runSync = bindings.runSync;
