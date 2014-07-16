var glNative = require('bindings')('mapbox_gl_native')

var x = 0
var y = 0
var z = 0

glNative.renderTile(x,y,z,function (tileImage) {
  console.log(tileImage)
})

