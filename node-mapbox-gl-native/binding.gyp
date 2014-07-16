{
  "targets": [
    {
      "target_name": "mapbox_gl_native",
      "sources": [ "mapbox_gl_native.cc" ],
      "include_dirs": [ "<!(node -e \"require('nan')\")" ]
    }
  ]
}
