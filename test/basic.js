var equal = require('assert').equal,
    duktape = require('../index');

describe('Duktape', function () {

  it('should run a script (sync)', function (finished) {
    var script = "\
          function test() {\n\
            return 1+1;\n\
          }\n\
        ";
    var ret = duktape.runSync("test", "", script);
    equal(ret, 2)
    finished();
  });

  it('should run a script (async)', function (finished) {
    var script = "\
          function test() {\n\
            return 1+1;\n\
          }\n\
        ";
    duktape.run("test", "", script, function(error, ret) {
      equal(error, false);
      equal(ret, 2);
      finished();
    });
  });

    it('should fail if script is malformed (sync)', function (finished) {
    var script = "\
          function test() {\n\
            malformed script \
            return 1+1;\n\
          }\n\
        ";
    try
    {
      var ret = duktape.runSync("test", "", script);
    } catch (error) {
      finished();
      return;
    }
    assert(false);
  });

  it('should fail if script is malformed (async)', function (finished) {
    var script = "\
          function test() {\n\
            malformed script \
            return 1+1;\n\
          }\n\
        ";
    duktape.run("test", "", script, function(error, ret) {
      equal(error, true);
      finished();
    });
  });

  it('should fail if function name is not string (sync)', function (finished) {
    try
    {
      var ret = duktape.runSync(true, "", "");
    } catch (error) {
      equal(error, "TypeError: Wrong arguments");
      finished();
      return;
    }
    assert(false);
  });

  it('should fail if parameter is not string (sync)', function (finished) {
    try
    {
      var ret = duktape.runSync("test", 1, "");
    } catch (error) {
      equal(error, "TypeError: Wrong arguments");
      finished();
      return;
    }
    assert(false);
  });

  it('should fail if script is not string (sync)', function (finished) {
    try
    {
      var ret = duktape.runSync("test", "", 123);
    } catch (error) {
      equal(error, "TypeError: Wrong arguments");
      finished();
      return;
    }
    assert(false);
  });

  it('should fail if function name is not string (async)', function (finished) {
    try
    {
      var ret = duktape.run(true, "", "", function() {});
    } catch (error) {
      equal(error, "TypeError: Wrong arguments");
      finished();
      return;
    }
    assert(false);
  });

  it('should fail if parameter is not string (async)', function (finished) {
    try
    {
      var ret = duktape.run("", 1, "", function() {});
    } catch (error) {
      equal(error, "TypeError: Wrong arguments");
      finished();
      return;
    }
    assert(false);
  });

  it('should fail if script is not string (async)', function (finished) {
    try
    {
      var ret = duktape.run("", "", 123, function() {});
    } catch (error) {
      equal(error, "TypeError: Wrong arguments");
      finished();
      return;
    }
    assert(false);
  });

  it('should fail if script callback is not specified (async)', function (finished) {
    try
    {
      var ret = duktape.run("", "", 123);
    } catch (error) {
      equal(error, "TypeError: Wrong number of arguments");
      finished();
      return;
    }
    assert(false);
  });

  it('should fail if script callback is not a function (async)', function (finished) {
    try
    {
      var ret = duktape.run("", "", "", "");
    } catch (error) {
      equal(error, "TypeError: Wrong arguments");
      finished();
      return;
    }
    assert(false);
  });

  it('should accept a single string parameter', function (finished) {
    var script = "\
          function test(paramString) {\n\
            return 1 + parseInt(paramString);\n\
          }\n\
        ";
    var ret = duktape.runSync("test", "5", script);
    equal(ret, 6);
    finished();
  });

  it('should return a single stringified JSON-object', function (finished) {
    var script = "\
          function test(paramString) {\n\
            return {retVal: paramString};\n\
          }\n\
        ";
    var param = "hello";
    var ret = duktape.runSync("test", param, script);
    equal(ret, JSON.stringify({ retVal: param }));
    finished();
  });

});
