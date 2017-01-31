/* GStreamer
 * Copyright (C) 2016 FIXME <fixme@example.com>
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
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gstmiuncamerachangedetector
 *
 * The miuncamerachangedetector element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v fakesrc ! miuncamerachangedetector ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#define MIUN_INPUT_OFFSET 4
#define MIUN_ANALYTIC 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include "gstmiuncamerachangedetector.h"
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include "harris.h"
#include <math.h>
#include <inttypes.h>
#include <string.h>
#if MIUN_ANALYTIC
    #include "harris_uint8.h"
#endif



GST_DEBUG_CATEGORY_STATIC (gst_miuncamerachangedetector_debug_category);
#define GST_CAT_DEFAULT gst_miuncamerachangedetector_debug_category

/* prototypes */


static void gst_miuncamerachangedetector_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_miuncamerachangedetector_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_miuncamerachangedetector_dispose (GObject * object);
static void gst_miuncamerachangedetector_finalize (GObject * object);

static gboolean gst_miuncamerachangedetector_start (GstBaseTransform * trans);
static gboolean gst_miuncamerachangedetector_stop (GstBaseTransform * trans);
static gboolean gst_miuncamerachangedetector_set_info (GstVideoFilter * filter, GstCaps * incaps,
    GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info);
static GstFlowReturn gst_miuncamerachangedetector_transform_frame (GstVideoFilter * filter,
    GstVideoFrame * inframe, GstVideoFrame * outframe);
static GstFlowReturn gst_miuncamerachangedetector_transform_frame_ip (GstVideoFilter * filter,
    GstVideoFrame * frame);

enum
{
  PROP_0
};


/* pad templates */

/* FIXME: add/remove formats you can handle */
#define VIDEO_SRC_CAPS \
    GST_VIDEO_CAPS_MAKE("{ GRAY8 }")

/* FIXME: add/remove formats you can handle */
#define VIDEO_SINK_CAPS \
    GST_VIDEO_CAPS_MAKE("{ GRAY8 }")


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstMiunCameraChangeDetector, gst_miuncamerachangedetector, GST_TYPE_VIDEO_FILTER,
  GST_DEBUG_CATEGORY_INIT (gst_miuncamerachangedetector_debug_category, "miuncamerachangedetector", 0,
  "debug category for miuncamerachangedetector element"));

static void
gst_miuncamerachangedetector_class_init (GstMiunCameraChangeDetectorClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS (klass);
  GstVideoFilterClass *video_filter_class = GST_VIDEO_FILTER_CLASS (klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS(klass),
      gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
        gst_caps_from_string (VIDEO_SRC_CAPS)));
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS(klass),
      gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
        gst_caps_from_string (VIDEO_SINK_CAPS)));

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS(klass),
      "FIXME Long name", "Generic", "FIXME Description",
      "FIXME <fixme@example.com>");

  gobject_class->set_property = gst_miuncamerachangedetector_set_property;
  gobject_class->get_property = gst_miuncamerachangedetector_get_property;
  gobject_class->dispose = gst_miuncamerachangedetector_dispose;
  gobject_class->finalize = gst_miuncamerachangedetector_finalize;
  base_transform_class->start = GST_DEBUG_FUNCPTR (gst_miuncamerachangedetector_start);
  base_transform_class->stop = GST_DEBUG_FUNCPTR (gst_miuncamerachangedetector_stop);
  video_filter_class->set_info = GST_DEBUG_FUNCPTR (gst_miuncamerachangedetector_set_info);
  video_filter_class->transform_frame = GST_DEBUG_FUNCPTR (gst_miuncamerachangedetector_transform_frame);
  video_filter_class->transform_frame_ip = GST_DEBUG_FUNCPTR (gst_miuncamerachangedetector_transform_frame_ip);

}

static void
gst_miuncamerachangedetector_init (GstMiunCameraChangeDetector *miuncamerachangedetector)
{
    miuncamerachangedetector->ctr = 0;
    miuncamerachangedetector->poi = 0;
    miuncamerachangedetector->unstableTill = 0;
}

void
gst_miuncamerachangedetector_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstMiunCameraChangeDetector *miuncamerachangedetector = GST_MIUNCAMERACHANGEDETECTOR (object);

  GST_DEBUG_OBJECT (miuncamerachangedetector, "set_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_miuncamerachangedetector_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstMiunCameraChangeDetector *miuncamerachangedetector = GST_MIUNCAMERACHANGEDETECTOR (object);

  GST_DEBUG_OBJECT (miuncamerachangedetector, "get_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_miuncamerachangedetector_dispose (GObject * object)
{
  GstMiunCameraChangeDetector *miuncamerachangedetector = GST_MIUNCAMERACHANGEDETECTOR (object);

  GST_DEBUG_OBJECT (miuncamerachangedetector, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_miuncamerachangedetector_parent_class)->dispose (object);
}

void
gst_miuncamerachangedetector_finalize (GObject * object)
{
  GstMiunCameraChangeDetector *miuncamerachangedetector = GST_MIUNCAMERACHANGEDETECTOR (object);

  GST_DEBUG_OBJECT (miuncamerachangedetector, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_miuncamerachangedetector_parent_class)->finalize (object);
}

static gboolean
gst_miuncamerachangedetector_start (GstBaseTransform * trans)
{
  GstMiunCameraChangeDetector *miuncamerachangedetector = GST_MIUNCAMERACHANGEDETECTOR (trans);

  GST_DEBUG_OBJECT (miuncamerachangedetector, "start");

  return TRUE;
}

static gboolean
gst_miuncamerachangedetector_stop (GstBaseTransform * trans)
{
  GstMiunCameraChangeDetector *miuncamerachangedetector = GST_MIUNCAMERACHANGEDETECTOR (trans);

  GST_DEBUG_OBJECT (miuncamerachangedetector, "stop");

  return TRUE;
}

static gboolean
gst_miuncamerachangedetector_set_info (GstVideoFilter * filter, GstCaps * incaps,
    GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info)
{
  GstMiunCameraChangeDetector *miuncamerachangedetector = GST_MIUNCAMERACHANGEDETECTOR (filter);

  GST_DEBUG_OBJECT (miuncamerachangedetector, "set_info");

  return TRUE;
}

/**
 * Transforms 64bit Image into positionsOfInterest (output)
 */
static void summarizeBlocks(GstMiunCameraChangeDetector *miuncamerachangedetector, buffer_t* input, buffer_t* output) {
    uint16_t i,j,k,l;
    BlockSummaryEntry *dest = miuncamerachangedetector->poi;
    int64_t
        *srci = (int64_t*)input->host,
        *srcj,*srck,*srcl;
    uint8_t *outi = output->host,
        *outj,*outk,*outl;

    
    int32_t width = input->extent[0],
        height = input->extent[1];
    
    uint16_t blockWidth = 100,
        blockHeight = 100;
    
    
    FILE * fp = fopen("poi.data", "a");
    miuncamerachangedetector->poiLength = 0;
    for (i = 0; i < height - blockHeight; i+= blockHeight) {
        srcj = srci;
        outj = outi;
        
        for (j = 0; j < width - blockWidth; j += blockWidth) {
            srck = srcj;
            outk = outj;
            BlockSummaryEntry current = {0};
            //dest->value = 0; // reset value
            for(k = 0; k < blockHeight; k++) {
                srcl = srck;
                outl = outk;
                
                for(l = 0; l < blockWidth; l++) {
                    if(abs(*srcl) > current.value) {
                        current.value = *srcl;
                        current.x = j + l;
                        current.y = i + k;
                    }
                    
                    //if(((i/blockHeight)+(j/blockWidth))%2) {
                    //    int64_t v = (*srcl)>>40;
                    //    *(outl) = (uint8_t) MIN(255, MAX(0,v));
                    //}
                    
                    srcl++;
                    outl++;
                }
                srck+= input->stride[1];
                outk+= output->stride[1];
            }
            
            if(current.value > 1<<10) {
                miuncamerachangedetector->poiLength++;
                *(dest++) = current;
                //fprintf(fp, "Point: %" PRId32 ":%" PRId32, current.x, current.y);
            }
            srcj += blockWidth;
            outj += blockWidth;
        }
        //printf("J:%u", j); // 1200
        outi += output->stride[1] * blockHeight;
        srci += input->stride[1] * blockHeight;

    }
    *dest = (BlockSummaryEntry) {0};
    //fprintf(fp, "EndSummarize %u\n\n\n", tmpCtr);
    fclose(fp);
}

static void calculatePoi(GstMiunCameraChangeDetector *miuncamerachangedetector, buffer_t* input, buffer_t* output)
{
    buffer_t halideBuffer = {0};
    uint16_t
        width = input->extent[0] - MIUN_INPUT_OFFSET * 2,
        height = input->extent[1] - MIUN_INPUT_OFFSET * 2;
    // @todo Avoid malloc&free foreach frame
    halideBuffer.host = (uint8_t*)malloc(width * height * sizeof(int64_t));
    halideBuffer.stride[0] = 1;
    halideBuffer.stride[1] = width;
    halideBuffer.extent[0] = width;
    halideBuffer.extent[1] = height;
    halideBuffer.elem_size = sizeof(int64_t); // 64bit
    halideBuffer.min[0] = halideBuffer.min[1] = MIUN_INPUT_OFFSET;
    
    if(!halideBuffer.host) {
        printf("Cannot handle image\n");
        return;
    }
    harris(input, &halideBuffer);
    
    /*
    int64_t* src = (int64_t*) halideBuffer.host;
    uint8_t* dst = output->host;
    
    uint32_t x,y;
    for(y=0;y<height;y++) {
        for(x=0;x<width;x++) {
            int64_t v = (*(src++))>>40;
            *(dst++) = (uint8_t) MIN(255, MAX(0,v));
        }
        src += input->stride[1] - input->extent[0];
        dst += output->stride[1] - output->extent[0];
    }
    */
    
    summarizeBlocks(miuncamerachangedetector, &halideBuffer, output);
    
    free(halideBuffer.host);
}

static void checkPoiInNewImage(GstMiunCameraChangeDetector *miuncamerachangedetector, buffer_t *input, buffer_t* output)
{
    buffer_t halideBuffer = {0};
    // @todo Avoid malloc&free foreach frame
    int64_t buff;
    
    halideBuffer.host = (uint8_t*) &buff;
    halideBuffer.stride[0] = 1;
    halideBuffer.stride[1] = 1;
    halideBuffer.extent[0] = 1;
    halideBuffer.extent[1] = 1;
    halideBuffer.elem_size = sizeof(int64_t); // 64bit
    
    BlockSummaryEntry* bs = miuncamerachangedetector->poi;
    
    int i;
    uint16_t matchingPoints = 0;
    #if MIUN_ANALYTIC
        FILE * poiFp = fopen("poi.data", "a");
        fprintf(poiFp, "Image : %u\n", miuncamerachangedetector->ctr);
    #endif
    for(i = 0; i < miuncamerachangedetector->poiLength; i++,bs++) {
        halideBuffer.min[0] = bs->x + MIUN_INPUT_OFFSET;
        halideBuffer.min[1] = bs->y + MIUN_INPUT_OFFSET;
        harris(input, &halideBuffer);

        if(buff > (bs->value*3)/4) {
            matchingPoints++;
#if MIUN_ANALYTIC
            fprintf(poiFp, "%u %u 1\n", bs->x, bs->y);
        } else {
            fprintf(poiFp, "%u %u 0\n", bs->x, bs->y);
#endif
        }
    }
#if MIUN_ANALYTIC
    fprintf(poiFp, "\n\n");
    fclose(poiFp);
    
    FILE * matchesFp = fopen("matches.data", "a");
    fprintf(
        matchesFp,
        "%u,%u,%u,%u\n",
        miuncamerachangedetector->ctr,
        matchingPoints,
        miuncamerachangedetector->poiLength,
        miuncamerachangedetector->poiLength - matchingPoints
    );
    
    fclose(matchesFp);
    
    FILE * changesFp = fopen("changes.data", "a");
#endif
    
    if(matchingPoints <= miuncamerachangedetector->poiLength/2) {
        if(!miuncamerachangedetector->unstableTill) {
            #if MIUN_ANALYTIC
                fprintf(changesFp, "%u", miuncamerachangedetector->ctr);
            #endif
        }
        miuncamerachangedetector->lastPoiLength = matchingPoints;
        miuncamerachangedetector->unstableTill = 2;
        calculatePoi(miuncamerachangedetector, input, output);
    } else if(miuncamerachangedetector->unstableTill) {
        if(abs(miuncamerachangedetector->lastPoiLength - matchingPoints) > 1) {
            miuncamerachangedetector->unstableTill = 2;
        } else {
            miuncamerachangedetector->unstableTill--;
            if(!miuncamerachangedetector->unstableTill) {
                #if MIUN_ANALYTIC
                fprintf(changesFp, " %u\n", miuncamerachangedetector->ctr);
                #endif
            }
        }
        miuncamerachangedetector->lastPoiLength = matchingPoints;
    }
#if MIUN_ANALYTIC
    fclose(changesFp);
#endif
}


/* transform */
static GstFlowReturn
gst_miuncamerachangedetector_transform_frame (GstVideoFilter * filter, GstVideoFrame * inframe,
    GstVideoFrame * outframe)
{
  //  guint n_planes = inframe->info.finfo->n_planes;
  GstMiunCameraChangeDetector *miuncamerachangedetector = GST_MIUNCAMERACHANGEDETECTOR (filter);
  GST_DEBUG_OBJECT (miuncamerachangedetector, "transform_frame");
  
    
    /*
   printf("Size %" PRId32 ":%" PRId32 "\n", inframe->info.stride[0], outframe->info.stride[0]);
     
   guint w,h,j;
   guint8 *sp, *dp;
  
  w = GST_VIDEO_FRAME_COMP_WIDTH (outframe,0) * GST_VIDEO_FRAME_COMP_PSTRIDE (outframe, 0);
  h = GST_VIDEO_FRAME_COMP_HEIGHT(outframe, 0);
  sp = inframe->data[0];
  dp = outframe->data[0];
  for (j = 0; j < h; j++) {
        memcpy (dp, sp, w);
        dp += outframe->info.stride[0];
        sp += inframe->info.stride[0];
    }
   */
    
  int i;
  GstVideoFrame* gstFrames[2] = {inframe, outframe};
  buffer_t halideBuffers[2] = {{0},{0}};
    
  for(i = 0; i < 2; i++) {
    halideBuffers[i].host = (uint8_t*)gstFrames[i]->data[0];
    halideBuffers[i].stride[0] = 1;
    halideBuffers[i].stride[1] = gstFrames[i]->info.stride[0];
    halideBuffers[i].extent[0] = gstFrames[i]->info.width;
    halideBuffers[i].extent[1] = gstFrames[i]->info.height;
    halideBuffers[i].elem_size = 1; // 8Bit
  }

    
  halideBuffers[1].min[0] = halideBuffers[1].min[1] = MIUN_INPUT_OFFSET;
  halideBuffers[1].extent[0] -= MIUN_INPUT_OFFSET * 2;
  halideBuffers[1].extent[1] -= MIUN_INPUT_OFFSET * 2;

#if MIUN_ANALYTIC
  harris_uint8(&halideBuffers[0], &halideBuffers[1]);
#endif
    
  if(!miuncamerachangedetector->poi) {
      uint16_t blockWidth = 100,
        blockHeight = 100,
        blocksX = inframe->info.width / blockWidth,
        blocksY = inframe->info.height / blockHeight;

      
      miuncamerachangedetector->poi = (BlockSummaryEntry*) malloc(sizeof(BlockSummaryEntry) * blocksX * blocksY);
      miuncamerachangedetector->poiHeight = blocksY;
      miuncamerachangedetector->poiWidth = blocksX;
      
      calculatePoi(miuncamerachangedetector, halideBuffers, halideBuffers+1);
  } else {
      checkPoiInNewImage(miuncamerachangedetector, halideBuffers, halideBuffers+1);
  }
  
  miuncamerachangedetector->ctr++;
    
  /*
  int i, j,k,l;
  const int
    width = inframe->info.width,
    height = inframe->info.height,
    blocksize = 40,
    blockWidth = width / blocksize,
    blockHeight = height / blocksize;
  */
    
  
/* Blocks
  FILE * fp = fopen("bump_blur.data", "a");
    
  guint8 *desti = (guint8 *) outframe->data[0];
  guint8 *srci = (guint8 *) inframe->data[0];
  guint8 *srcj,*srck,*srcl,
    *destj, *destk, *destl;
  for (j = 0; j < height; j+= blockHeight) {
    gint32 lineblur = 0;
    destj = desti;
    srcj = srci;
    
    for (i = 0; i < width; i += blockWidth) {
        srck = srcj;
        destk = destj;
        for(k = 0; k < blockHeight; k++) {
            srcl = srck;
            destl = destk;
            
            for(l = 0; l < blockWidth; l++) {
                *destl = *srcl;
                srcl++;
                destl++;
            }
            srck+= inframe->info.stride[0];
            destk+= inframe->info.stride[0];
        }
        srcj += blockWidth;
        destj += blockWidth;
    }
    srci += inframe->info.stride[0] * blockHeight;
    desti += inframe->info.stride[0] * blockHeight;
      
    fprintf(fp, "%i;", lineblur);
  }
  fprintf(fp, "\n");
    
  fclose(fp);
 */

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_miuncamerachangedetector_transform_frame_ip (GstVideoFilter * filter, GstVideoFrame * frame)
{
  GstMiunCameraChangeDetector *miuncamerachangedetector = GST_MIUNCAMERACHANGEDETECTOR (filter);

  GST_DEBUG_OBJECT (miuncamerachangedetector, "transform_frame_ip");

  return GST_FLOW_OK;
}

static gboolean
plugin_init (GstPlugin * plugin)
{

  /* FIXME Remember to set the rank if it's an element that is meant
     to be autoplugged by decodebin. */
  return gst_element_register (plugin, "miuncamerachangedetector", GST_RANK_SECONDARY,
      GST_TYPE_MIUNCAMERACHANGEDETECTOR);
}

/* FIXME: these are normally defined by the GStreamer build system.
   If you are creating an element to be included in gst-plugins-*,
   remove these, as they're always defined.  Otherwise, edit as
   appropriate for your external plugin package. */
#ifndef VERSION
#define VERSION "0.0.FIXME"
#endif
#ifndef PACKAGE
#define PACKAGE "FIXME_package"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "FIXME_package_name"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "http://FIXME.org/"
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    miuncamerachangedetector,
    "FIXME plugin description",
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)

