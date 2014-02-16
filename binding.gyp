{

	"targets": [
		{
			"target_name": "duktape",
			"dependencies": ["libduktape"],
			"include_dirs": [
				"lib/duktape",
				"src"
			],
			"sources": [ 
				"src/duktapejs.cpp", 
				"src/duktapevm.cpp",
				"src/callbackcache.cpp"
			],
			"conditions": [
				["OS=='linux'", {
					"cflags": [
						"-O2",
						"-std=c++11",
						"-pedantic-errors",
						"-Wall",
						"-fstrict-aliasing",
						"-fomit-frame-pointer"
					]
				}]
			]
		},
		{
			"target_name": "libduktape",
			"type": "static_library",
			"include_dirs": [
				"lib/duktape"
			],
			"sources": [ 
				"lib/duktape/duktape.c"
			],
			"conditions": [
				["OS=='linux'", {
					"cflags": [
						"-O2",
						"-std=c99",
						"-pedantic-errors",
						"-Wall",
						"-fstrict-aliasing",
						"-fomit-frame-pointer"
					]
				}]
			]
		},
	],
}
