/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 * 2023 Beaver, Plastic Wrap

Test this plugin without installing by pasting this syntax into Gimp's GEGL graph filter.
This code will generate a static preview of the plugin

id=x 
over aux=[ ref=x noise-reduction iterations=2  gaussian-blur std-dev-x=1 std-dev-y=1  ]
gimp:layer-mode layer-mode=normal opacity=0.11  aux=[ ref=x  gaussian-blur std-dev-x=7 std-dev-y=7 emboss depth=98 elevation=30 azimuth=4  emboss depth=20  elevation=40 ]
mean-curvature-blur iterations=2
id=y
gimp:layer-mode layer-mode=softlight opacity=0.50 aux=[ ref=y ]

--end of syntax --

Fun fact, I've been wanting to make a filter like this for over a year and had some idea with my bevel fused with hard light but it was not until today (oct 3 2023) that I figured out a plastic wrap algorithm that looks good. Yes, this filter is inspired by Abobe Photoshop's plastic wrap.

 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (opacity, _("Opacity of plastic"), 0.14)
   description (_("Opacity of the plastic wrap"))
   value_range (0.10, 1.00)
   ui_range    (0.10, 0.30)
   ui_gamma    (3.0)
  ui_steps      (0.1, 0.50)


property_int (smoothcontent, _("Smooth content below the plastic"), 3)
   description (_("Apply Gimp's noise reduction filter on the subject/object that is inside on the plastic"))
   value_range (1, 10)
   ui_range    (1, 10)
   ui_gamma    (3.0)

property_double (blurcontent, _("Blur content below the plastic"), 0.0)
   description (_("Apply Gimp's gaussian blur filter on the subject/object that is inside on the plastic. At 0 this is entirely disabled."))
   value_range (0.0, 2.0)
   ui_range    (0.0, 2.0)
   ui_gamma    (3.0)
  ui_steps      (0.1, 0.50)


property_double (tightness, _("Plastic wrap control"), 4.8)
   description (_("Metaphorically, on lower values the plastic will be more tightly wrapped, on higher values it will leave some air in the bag. This is done by a internal gaussian blur. On small images this should be low, on larger images this should be high."))
   value_range (2.0, 15.0)
   ui_range    (2.0, 15.0)
   ui_gamma    (3.0)
  ui_steps      (0.1, 0.50)

property_double (azimuth, _("Plastic azimuth"), 3.0)
   description (_("Emboss Azimuth for Plastic"))
   value_range (3.0, 90.0)
   ui_range    (3.0, 90.0)
   ui_gamma    (3.0)
  ui_steps      (0.1, 0.50)


property_double (elevation, _("Plastic elevation"), 80.0)
   description (_("Emboss elevation for Plastic. Rotate the brightest pixels with this."))
   value_range (30.0, 90.0)
   ui_range    (30.0, 90.0)
   ui_gamma    (3.0)
  ui_steps      (0.1, 0.50)



property_double (elevation2, _("Faint Plastic elevation"), 20.0)
   description (_("Emboss elevation for a second faint emboss, that makes the plastic look better. This rotates the brightest pixels"))
   value_range (10.0, 90.0)
   ui_range    (10.0, 90.0)
   ui_gamma    (3.0)
  ui_steps      (0.1, 0.50)


property_int (depth, _("Plastic depth"), 66.0)
   description (_("Emboss depth control of plastic"))
   value_range (60, 100)
   ui_range    (60, 100)
   ui_gamma    (3.0)

property_int (depth2, _("Faint plastic depth"), 20.0)
   description (_("Emboss depth control of the faint plastic "))
   value_range (5, 40)
   ui_range    (5, 40)
   ui_gamma    (3.0)



property_int (smoothall, _("Mean Curvature smooth everything"), 2)
   description (_("Apply Gimp's Mean Curvature Blur filter on everything."))
   value_range (0, 6)
   ui_range    (0, 6)
   ui_gamma    (3.0)

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     plasticwrapping
#define GEGL_OP_C_SOURCE plastic_wrapping.c

#include "gegl-op.h"

static void attach (GeglOperation *operation)
{
  GeglNode *gegl = operation->node;
  GeglNode *input, *idref, *over, *normal, *mcb, *sl, *nr, *blur, *gaussian, *alphalockreplaceblendmode, *alphalockreplaceblendmode2, *idref2, *idref3,  *emboss, *emboss2, *output;

  input    = gegl_node_get_input_proxy (gegl, "input");
  output   = gegl_node_get_output_proxy (gegl, "output");

  idref = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

  over = gegl_node_new_child (gegl,
                                  "operation", "gegl:over",
                                  NULL);

/*This is a GEGL exclusive blend mode that works like Gimp's alpha lock and replace blend mode fused together. It is meant to alpha lock a gaussian blur in this case.
There is no Gimp only blend mode that does anything like this. That is what makes it unique and interesting.*/
  alphalockreplaceblendmode = gegl_node_new_child (gegl,
                                  "operation", "gegl:src-in",
                                  NULL);

  alphalockreplaceblendmode2 = gegl_node_new_child (gegl,
                                  "operation", "gegl:src-in",
                                  NULL);

  idref3 = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);


  idref2 = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

normal = gegl_node_new_child (gegl,
                                    "operation", "gimp:layer-mode", "layer-mode", 28,  NULL);

  mcb = gegl_node_new_child (gegl,
                                  "operation", "gegl:mean-curvature-blur", "iterations", 2,
                                  NULL);
#define softlight \
" id=y gimp:layer-mode layer-mode=softlight opacity=0.50 aux=[ ref=y ]"\

  sl = gegl_node_new_child (gegl,
                                  "operation", "gegl:gegl", "string", softlight,
                                  NULL);

  nr = gegl_node_new_child (gegl,
                                  "operation", "gegl:noise-reduction", "iterations", 2,
                                  NULL);

  blur = gegl_node_new_child (gegl,
                                  "operation", "gegl:gaussian-blur", "std-dev-x", 1.0, "std-dev-y", 1.0,
                                  NULL);

  gaussian = gegl_node_new_child (gegl,
                                  "operation", "gegl:gaussian-blur", "std-dev-x", 7.0, "std-dev-y", 7.0,
                                  NULL);

  emboss = gegl_node_new_child (gegl,
                                  "operation", "gegl:emboss", "depth", 98, "elevation", 30.0, "azimuth", 4.0,
                                  NULL);

  emboss2 = gegl_node_new_child (gegl,
                                  "operation", "gegl:emboss", "depth", 20, "elevation", 40.0,
                                  NULL);

/*If you are interested in developing GEGL plugins pay attention to how the same gegl:nop node (idref) is used 3 times to match the gegl syntax on the top of the page. nop is both the id and ref */
  gegl_node_link_many (input, idref, over, normal, mcb, sl, output, NULL);
  gegl_node_connect (over, "aux", alphalockreplaceblendmode, "output");
  gegl_node_link_many (idref, nr, idref2, alphalockreplaceblendmode,  NULL);
  gegl_node_link_many (idref2, blur, NULL);
  gegl_node_connect (alphalockreplaceblendmode, "aux", blur, "output");
  gegl_node_connect (normal, "aux", emboss2, "output");
  gegl_node_link_many (idref, idref3, alphalockreplaceblendmode2, emboss, emboss2,  NULL);
  gegl_node_connect (alphalockreplaceblendmode2, "aux", gaussian, "output");
  gegl_node_link_many (idref3, gaussian,  NULL);

 gegl_operation_meta_redirect (operation, "smoothcontent", nr, "iterations"); 
 gegl_operation_meta_redirect (operation, "blurcontent", blur, "std-dev-x"); 
 gegl_operation_meta_redirect (operation, "blurcontent", blur, "std-dev-y"); 
 gegl_operation_meta_redirect (operation, "tightness", gaussian, "std-dev-x"); 
 gegl_operation_meta_redirect (operation, "tightness", gaussian, "std-dev-y"); 
 gegl_operation_meta_redirect (operation, "elevation", emboss, "elevation"); 
 gegl_operation_meta_redirect (operation, "depth", emboss, "depth"); 
 gegl_operation_meta_redirect (operation, "depth2", emboss2, "depth"); 
 gegl_operation_meta_redirect (operation, "azimuth", emboss, "azimuth"); 
 gegl_operation_meta_redirect (operation, "smoothall", mcb, "iterations"); 
 gegl_operation_meta_redirect (operation, "elevation2", emboss2, "elevation"); 
 gegl_operation_meta_redirect (operation, "opacity", normal, "opacity"); 

}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;

  gegl_operation_class_set_keys (operation_class,
    "name",        "lb:plastic",
    "title",       _("Plastic Wrap"),
    "reference-hash", "e4hklrgl34plafuhgsticuwrar3p",
    "description", _("An effect that makes it look like the content of your image was covered in plastic wrap. This works best on alpha channel present images."),
    "gimp:menu-path", "<Image>/Filters/Light and Shadow",
    "gimp:menu-label", _("Plastic Wrap..."),
    NULL);
}

#endif
