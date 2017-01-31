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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _GST_MIUNCAMERACHANGEDETECTOR_H_
#define _GST_MIUNCAMERACHANGEDETECTOR_H_

#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include <stdint.h>

G_BEGIN_DECLS

#define GST_TYPE_MIUNCAMERACHANGEDETECTOR   (gst_miuncamerachangedetector_get_type())
#define GST_MIUNCAMERACHANGEDETECTOR(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_MIUNCAMERACHANGEDETECTOR,GstMiunCameraChangeDetector))
#define GST_MIUNCAMERACHANGEDETECTOR_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_MIUNCAMERACHANGEDETECTOR,GstMiunCameraChangeDetectorClass))
#define GST_IS_MIUNCAMERACHANGEDETECTOR(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_MIUNCAMERACHANGEDETECTOR))
#define GST_IS_MIUNCAMERACHANGEDETECTOR_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_MIUNCAMERACHANGEDETECTOR))

typedef struct _GstMiunCameraChangeDetector GstMiunCameraChangeDetector;
typedef struct _GstMiunCameraChangeDetectorClass GstMiunCameraChangeDetectorClass;


typedef struct _BlockSummaryEntry {
    int64_t value;
    int16_t x;
    int16_t y;
} BlockSummaryEntry;

struct _GstMiunCameraChangeDetector
{
  GstVideoFilter base_miuncamerachangedetector;
  guint16 ctr;
  BlockSummaryEntry* poi;
  uint32_t poiWidth;
  uint32_t poiHeight;
  int16_t poiLength;
  uint8_t unstableTill;
};

struct _GstMiunCameraChangeDetectorClass
{
  GstVideoFilterClass base_miuncamerachangedetector_class;
};

GType gst_miuncamerachangedetector_get_type (void);

G_END_DECLS

#endif
