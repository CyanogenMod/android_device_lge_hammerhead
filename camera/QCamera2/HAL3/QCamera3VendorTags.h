/* Copyright (c) 2014, The Linux Foundataion. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of The Linux Foundation nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
* ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#ifndef __QCAMERA3VENDORTAGS_H__
#define __QCAMERA3VENDORTAGS_H__

namespace qcamera {

enum qcamera3_ext_section {
    QCAMERA3_PRIVATEDATA = VENDOR_SECTION,
    QCAMERA3_SECTIONS_END
};

enum qcamera3_ext_section_ranges {
    QCAMERA3_PRIVATEDATA_START = QCAMERA3_PRIVATEDATA << 16
};

enum qcamera3_ext_tags {
    QCAMERA3_PRIVATEDATA_REPROCESS = QCAMERA3_PRIVATEDATA_START,
    QCAMERA3_PRIVATEDATA_END
};

class QCamera3VendorTags {

public:
    static void get_vendor_tag_ops(vendor_tag_ops_t* ops);
    static int get_tag_count(
            const vendor_tag_ops_t *ops);
    static void get_all_tags(
            const vendor_tag_ops_t *ops,
            uint32_t *tag_array);
    static const char* get_section_name(
            const vendor_tag_ops_t *ops,
            uint32_t tag);
    static const char* get_tag_name(
            const vendor_tag_ops_t *ops,
            uint32_t tag);
    static int get_tag_type(
            const vendor_tag_ops_t *ops,
            uint32_t tag);

    static const vendor_tag_ops_t *Ops;
};

}; // namespace qcamera

#endif /* __QCAMERA3VENDORTAGS_H__ */
