{

	"targets": [
		{
			"target_name": "duktape",
			"dependencies": ["libduktape"],
			"include_dirs": [
				"lib/duktape/src",
				"src"
			],
			"sources": [ 
				"src/duktape_node_main.cpp", 
				"src/duktapevm.cpp",
				"src/callbackcache.cpp",
				"src/run_sync.cpp",
				"src/run_async.cpp"
			],
			"conditions": [
				["OS=='linux'", {
					"cflags": [
						"-O2",
						"-std=c++0x",
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
				"lib/duktape/src"
			],
			"sources": [ 
				"lib/duktape/src/duktape.c"
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
