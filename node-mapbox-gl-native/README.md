### Build & Run

1. `cd` up to the mapbox-gl-native root directory
2. `make node-tileserver`
3. `scripts/node-tileserver.sh`

This does not do anything useful yet, but:

 1. It's building a node module, with llmr as a dependency, in the correct
    build dir using the main Makefile to invoke `node-gyp rebuild` by way
    of `npm install`.

 2. The (for now useless) node script which uses the addon module can
    be invoked with a script which follows the pattern of the existing
    invocation scripts.
