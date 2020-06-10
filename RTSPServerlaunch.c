/* GStreamer
 * Copyright (C) 2008 Wim Taymans <wim.taymans at gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */


/*To compile use:
gcc -o test-launch RTSPServerlaunch.c  `pkg-config --cflags --libs gstreamer-rtsp-server-1.0`
*/

#include <gst/gst.h>

#include <stdio.h>

#include <gst/rtsp-server/rtsp-server.h>

#define RTSP_PORT 8000
#define UDPSINK_PORT 5400
#define RTSP_MOUNT_POINT "/mp4"
#define USERNAME "SCrobo"
#define PASSWORD "roboVision"

static gboolean
remove_func (GstRTSPSessionPool * pool, GstRTSPSession * session,
    GstRTSPServer * server)
{
  return GST_RTSP_FILTER_REMOVE;
}

static gboolean
remove_sessions (GstRTSPServer * server)
{
  GstRTSPSessionPool *pool;

  g_print ("removing all sessions\n");
  pool = gst_rtsp_server_get_session_pool (server);
  gst_rtsp_session_pool_filter (pool,
      (GstRTSPSessionPoolFilterFunc) remove_func, server);
  g_object_unref (pool);

  return FALSE;
}

static gboolean
timeout (GstRTSPServer * server)
{
  GstRTSPSessionPool *pool;

  pool = gst_rtsp_server_get_session_pool (server);
  gst_rtsp_session_pool_cleanup (pool);
  g_object_unref (pool);

  return TRUE;
}

int
main (int argc, char *argv[])
{
  GMainLoop *loop;
  GstRTSPServer *server;
  GstRTSPMountPoints *mounts;
  GstRTSPMediaFactory *factory;
  GstRTSPAuth *auth;
  GstRTSPToken *token;
  gchar *basic;
  char udpsrc_pipeline[512];
  char port_num_Str[64] = { 0 };
  char *encoder_name;

  encoder_name = "H264";
  
  sprintf (udpsrc_pipeline,
      "( udpsrc name=pay0 port=%d caps=\"application/x-rtp, media=video, "
      "clock-rate=90000, encoding-name=H264, payload=96 \" )", UDPSINK_PORT);

  sprintf (port_num_Str, "%d", RTSP_PORT);

  gst_init (&argc, &argv);

  loop = g_main_loop_new (NULL, FALSE);

  /* create a server instance */
  server = gst_rtsp_server_new ();

  /* get the mounts for this server, every server has a default mapper object
   * that be used to map uri mount points to media factories */
  mounts = gst_rtsp_server_get_mount_points (server);


  /* make a media factory for a test stream. The default media factory can use
   * gst-launch syntax to create pipelines. 
   * any launch line works as long as it contains elements named pay%d. Each
   * element with pay%d names will be a stream */
  factory = gst_rtsp_media_factory_new ();
  gst_rtsp_media_factory_set_launch (factory, udpsrc_pipeline);
  /* attach the test factory to the /test url */
  gst_rtsp_mount_points_add_factory (mounts, RTSP_MOUNT_POINT, factory);

  /* allow user and admin to access this resource */
  gst_rtsp_media_factory_add_role (factory, USERNAME,
      GST_RTSP_PERM_MEDIA_FACTORY_ACCESS, G_TYPE_BOOLEAN, TRUE,
      GST_RTSP_PERM_MEDIA_FACTORY_CONSTRUCT, G_TYPE_BOOLEAN, TRUE, NULL);
  gst_rtsp_media_factory_add_role (factory, "anonymous",
      GST_RTSP_PERM_MEDIA_FACTORY_ACCESS, G_TYPE_BOOLEAN, FALSE,
      GST_RTSP_PERM_MEDIA_FACTORY_CONSTRUCT, G_TYPE_BOOLEAN, FALSE, NULL);

  /* don't need the ref to the mapper anymore */
  g_object_unref (mounts);

  /* make a new authentication manager */
  auth = gst_rtsp_auth_new ();

  /* make default token, it has no permissions */
  token =
      gst_rtsp_token_new (GST_RTSP_TOKEN_MEDIA_FACTORY_ROLE, G_TYPE_STRING,
      "anonymous", NULL);
  gst_rtsp_auth_set_default_token (auth, token);
  gst_rtsp_token_unref (token);

  /* make user token */
  token =
      gst_rtsp_token_new (GST_RTSP_TOKEN_MEDIA_FACTORY_ROLE, G_TYPE_STRING,
      USERNAME, NULL);
  basic = gst_rtsp_auth_make_basic (USERNAME, PASSWORD);
  gst_rtsp_auth_add_basic (auth, basic, token);
  g_free (basic);
  gst_rtsp_token_unref (token);

  /* set as the server authentication manager */
  gst_rtsp_server_set_auth (server, auth);
  g_object_unref (auth);

  /* attach the server to the default maincontext */
  if (gst_rtsp_server_attach (server, NULL) == 0)
    goto failed;

  g_timeout_add_seconds (2, (GSourceFunc) timeout, server);
  g_timeout_add_seconds (10, (GSourceFunc) remove_sessions, server);

  /* start serving */
  g_print ("stream with %s:%s ready at rtsp://localhost:%s%s\n", USERNAME, PASSWORD, port_num_Str, RTSP_MOUNT_POINT);
  g_main_loop_run (loop);

  return 0;

  /* ERRORS */
failed:
  {
    g_print ("failed to attach the server\n");
    return -1;
  }
}
