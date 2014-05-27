/*jshint multistr: true */

var equal = require('assert').equal,
    duktape = require('../index');

describe('Duktape API', function () {

  var callbackFunc = function(param) {
    return param + "!";
  };

  it('host program should be able to provide an API (sync)', function (finished) {
    var script = "\
          function test() {\n\
            var ret = exclamate('hello'); \
            return ret;\n\
          }\n\
        ";
    var API = {
      exclamate: callbackFunc
    };

    var ret = duktape.runSync("test", "", script, API);
    equal(ret, "hello!");
    finished();
  });

  it('host program should be able to provide an API (async)', function (finished) {
    var script = "\
          function test() {\n\
            var ret = exclamate('hello'); \
            return ret;\n\
          }\n\
        ";
    var API = {
      exclamate: callbackFunc
    };

    duktape.run("test", "", script, API, function(error, ret) {
      equal(error, false);
      equal(ret, "hello!");
      finished();
    });
  });

  it('script should be able to pass a single string-serialized object-parameter to callback function', function (finished) {

    var callbackFunc2 = function(param) {
      var obj = JSON.parse(param);
      return obj.greeting + " " + obj.name + "!";
    };

    var script = "\
          function test() {\n\
            var parameters = { \
              greeting: 'hey', \
              name: 'world' \
            }; \
            var ret = greet(parameters); \
            return ret;\n\
          }\n\
        ";
    var API = {
      greet: callbackFunc2
    };

    var ret = duktape.runSync("test", "", script, API);
    equal(ret, "hey world!");
    finished();

  });


  it('API parameter should be empty when called with incompatible object', function (finished) {
    
    var callbackFunc2 = function(param) {
      return param;
    };

    var script = "\
          function test() {\n\
            var parameters = function() {}; \
            var ret = greet(parameters); \
            return ret;\n\
          }\n\
        ";
    var API = {
      greet: callbackFunc2
    };

    var ret = duktape.runSync("test", "", script, API);
    equal(ret, "");
    finished();

  });

  it('API functions should be callable multiple times from script (async)', function (finished) {
    var script = "\
          function test() {\n\
            var ret = exclamate('hello') + exclamate('hello') + exclamate('hello');\
            return ret;\n\
          }\n\
        ";
    var API = {
      exclamate: callbackFunc
    };

    duktape.run("test", "", script, API, function(error, ret) {
      equal(error, false);
      equal(ret, "hello!hello!hello!");
      finished();
    });
  });

  it('API functions should be callable multiple times from script (sync)', function (finished) {
    var script = "\
          function test() {\n\
            var ret = exclamate('hello') + exclamate('hello') + exclamate('hello');\
            return ret;\n\
          }\n\
        ";
    var API = {
      exclamate: callbackFunc
    };

    var ret = duktape.runSync("test", "", script, API);
    equal(ret, "hello!hello!hello!");
    finished();
  });
});
