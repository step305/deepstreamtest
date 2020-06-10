In deepstream_sdk_v4.0.2_jetson/sources/apps/apps_common/deepstream_sink_bin.c
change code in line #538 to (disable broadcast streaming on UDP):
g_object_set (G_OBJECT (bin->sink), "host", "127.0.0.1", "port",
      config->udp_port, "async", FALSE, "sync", 0, NULL);
      
launch with:
 - 4 cameras with ResNet10 detector:
    ./deep_stream_app -c conf_4cams.txt
 - USB camera with Face detector:
    ./deep_stream_app -c conf_face.txt
 - Combine streams and stream to UDP:
    ./deep_stream_app -c conf_main.txt
 
 
