duktape-node
=========

Simple [Duktape Javascript engine](http://duktape.org/) integration for node.js. 

This package provides facilities for running a single script on an isolated Duktape virtual machine.

## Installing
    npm install duktape
    
## API

### run(functionName, parameter, script, apiObject, callback)
**Description**

Runs a single script on duktape and returns error status and return value to callback.

**Parameters**

* functionName: function to run (string)
* parameter: parameter for function (string)
* script: whole javascript source of script (string)
* apiObject: API for script (object)
  * properties:
    * key: name for function
    * value: function to call
* callback: function with signature `function(error, returnValue)`
  * error: error status (boolean)
  * returnValue: return value from script (string) or error string in case of an error
  
**Example**
```javascript
var duktape = require('duktape');

var apiFunction = function(name) {
    return "hello " + name;
}

var script = " \
  function helloFun(parameterString) { \
    return { \
      value: hello(parameterString), \
      extra: 'bye ' + parameterString \
    }; \
  }";

var apiObject = {
    hello: apiFunction
};
  
duktape.run("helloFun", "world", script, apiObject, function(error, ret) {
  if(error) {
    console.log("got error: " + ret);
  } else {
    var retVal = JSON.parse(ret);
    console.log(retVal.value);
    console.log(retVal.extra);
  }
});
```

### runSync(functionName, parameter, script, apiObject)

**Description**

  Runs a single script on duktape and returns script's return value. Errors are handled by exceptions. Otherwise functionality is the same as in async-version.

**Parameters**

* functionName: function to run (string)
* parameter: parameter for function (string)
* script: whole javascript source of script (string)
* apiObject: API for script (object)
  * properties:
    * key: name for function
    * value: function to call

**Example**
```javascript
var duktape = require('duktape');

var apiFunction = function(name) {
    return "hello " + name;
}

var apiObject = {
    hello: apiFunction
};

var script = " \
  function helloFun(parameterString) { \
    return { \
      value: hello(parameterString), \
      extra: 'bye ' + parameterString \
    }; \
  }";
  
try {
  var ret = duktape.runSync("helloFun", "world", script, apiObject);
  var retVal = JSON.parse(ret);
  console.log(retVal.value);
  console.log(retVal.extra);
} catch(error) {
  console.log("got error" + error);
}

```

## What this package _doesn't_ do
This package just runs scripts on duktape VM without doing much else. Everything listed here is left for host programs responsibility.
* Limit resources (memory, cpu time, etc.)
* Input parameters and return values
  * Sanity checks
  * Formatting
  
## Known issues
* Probably does not compile on other compilers than gcc (has been tested on linux: gcc4.8.1, gcc4.6.4, gcc4.4.7).
* No async support from defined API-functions back to duktape (ie. no callbacks).
