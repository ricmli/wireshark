/* Packet-rdp_cliprdr.c
 * Routines for the clipboard redirection RDP channel
 * Copyright 2023, David Fort <contact@hardening-consulting.com>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * See: "[MS-RDPECLIP] "
 */

#include "config.h"

#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/conversation.h>
#include <epan/expert.h>
#include <epan/value_string.h>

#include "packet-rdpudp.h"

#define PNAME  "RDP clipboard redirection channel Protocol"
#define PSNAME "cliprdr"
#define PFNAME "rdp_cliprdr"

void proto_register_rdp_cliprdr(void);
void proto_reg_handoff_rdp_cliprdr(void);


static int proto_rdp_cliprdr;

static int hf_cliprdr_msgType;
static int hf_cliprdr_msgFlags;
static int hf_cliprdr_dataLen;

static int hf_cliprdr_requestedFormatId;
static int hf_cliprdr_clipDataId;
static int hf_cliprdr_streamId;
static int hf_cliprdr_lindex;
static int hf_cliprdr_dwFlags;
static int hf_cliprdr_nPositionLow;
static int hf_cliprdr_nPositionHigh;
static int hf_cliprdr_cbRequested;

static int ett_rdp_cliprdr;


enum {
	CB_MONITOR_READY = 0x0001,
	CB_FORMAT_LIST = 0x0002,
	CB_FORMAT_LIST_RESPONSE = 0x0003,
	CB_FORMAT_DATA_REQUEST = 0x0004,
	CB_FORMAT_DATA_RESPONSE = 0x0005,
	CB_TEMP_DIRECTORY = 0x0006,
	CB_CLIP_CAPS = 0x0007,
	CB_FILECONTENTS_REQUEST = 0x0008,
	CB_FILECONTENTS_RESPONSE = 0x0009,
	CB_LOCK_CLIPDATA = 0x000A,
	CB_UNLOCK_CLIPDATA = 0x000B,
};


static const value_string rdp_cliprdr_order_vals[] = {
	{ CB_MONITOR_READY, "Monitor ready"},
	{ CB_FORMAT_LIST, "Format list"},
	{ CB_FORMAT_LIST_RESPONSE, "Format list response"},
	{ CB_FORMAT_DATA_REQUEST, "Format data request"},
	{ CB_FORMAT_DATA_RESPONSE, "Format data response"},
	{ CB_TEMP_DIRECTORY, "Temporary directory"},
	{ CB_CLIP_CAPS, "Capabilities"},
	{ CB_FILECONTENTS_REQUEST, "File content request"},
	{ CB_FILECONTENTS_RESPONSE, "File content response"},
	{ CB_LOCK_CLIPDATA, "Lock clipdata"},
	{ CB_UNLOCK_CLIPDATA, "Unlock clipdata"},
	{ 0x0, NULL},
};

static const value_string msgFlags_vals[] = {
	{ 0x0000, "" },
	{ 0x0001, "CB_RESPONSE_OK" },
	{ 0x0002, "CB_RESPONSE_FAIL" },
	{ 0x0004, "CB_ASCII_NAMES" },
	{ 0x0, NULL},
};

static const value_string knownFormats_vals[] = {
	{ 0x0000, "CB_RAW" },
	{ 0x0001, "CF_TEXT" },
	{ 0x0002, "CF_BITMAP" },
	{ 0x0003, "CF_METAFILEPICT" },
	{ 0x0004, "CF_SYLK" },
	{ 0x0005, "CF_DIF" },
	{ 0x0006, "CF_TIFF" },
	{ 0x0007, "CF_OEMTEXT" },
	{ 0x0008, "CF_DIB" },
	{ 0x0009, "CF_PALETTE" },
	{ 0x000a, "CF_PENDATA" },
	{ 0x000b, "CF_RIFF" },
	{ 0x000c, "CF_WAVE" },
	{ 0x000d, "CF_UNICODETEXT" },
	{ 0x000e, "CF_ENHMETAFILE" },
	{ 0x000f, "CF_HDROP" },
	{ 0x0010, "CF_LOCALE" },
	{ 0x0011, "CF_DIBV5" },
	{ 0x0080, "CF_OWNERDISPLAY" },
	{ 0x0081, "CF_DSPTEXT" },
	{ 0x0082, "CF_DSPBITMAP" },
	{ 0x0083, "CF_DSPMETAFILEPICT" },
	{ 0x008E, "CF_DSPENHMETAFILE" },
	{ 0x0, NULL},
};


static int
dissect_rdp_cliprdr(tvbuff_t *tvb _U_, packet_info *pinfo, proto_tree *parent_tree _U_, void *data _U_)
{
	proto_item *item;
	int nextOffset, offset = 0;
	uint32_t cmdId = 0;
	uint32_t pduLength;
        uint32_t formatId;
	proto_tree *tree;

	parent_tree = proto_tree_get_root(parent_tree);
	col_set_str(pinfo->cinfo, COL_PROTOCOL, "CLIPRDR");
	col_clear(pinfo->cinfo, COL_INFO);

	pduLength = tvb_get_uint32(tvb, offset + 4, ENC_LITTLE_ENDIAN) + 8;
	nextOffset = offset + pduLength;

	item = proto_tree_add_item(parent_tree, proto_rdp_cliprdr, tvb, offset, pduLength, ENC_NA);
	tree = proto_item_add_subtree(item, ett_rdp_cliprdr);

	proto_tree_add_item_ret_uint(tree, hf_cliprdr_msgType, tvb, offset, 2, ENC_LITTLE_ENDIAN, &cmdId);
	offset += 2;

	proto_tree_add_item(tree, hf_cliprdr_msgFlags, tvb, offset, 2, ENC_LITTLE_ENDIAN);
	offset += 2;

	proto_tree_add_item(tree, hf_cliprdr_dataLen, tvb, offset, 4, ENC_LITTLE_ENDIAN);
	offset += 4;

	col_set_str(pinfo->cinfo, COL_INFO, val_to_str_const(cmdId, rdp_cliprdr_order_vals, "Unknown clipboard command"));

	switch (cmdId) {
	case CB_MONITOR_READY:
	case CB_FORMAT_LIST:
            break;
	case CB_FORMAT_LIST_RESPONSE:
            break;
	case CB_FORMAT_DATA_REQUEST:
            proto_tree_add_item_ret_uint(tree, hf_cliprdr_requestedFormatId, tvb, offset, 4, ENC_LITTLE_ENDIAN, &formatId);

            col_append_fstr(pinfo->cinfo, COL_INFO, " - %s", val_to_str_const(formatId, knownFormats_vals, "Unknown format"));
            break;
	case CB_FORMAT_DATA_RESPONSE:
            break;
	case CB_TEMP_DIRECTORY:
	case CB_CLIP_CAPS:
            break;
	case CB_FILECONTENTS_REQUEST:
            proto_tree_add_item(tree, hf_cliprdr_streamId, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;

            proto_tree_add_item(tree, hf_cliprdr_lindex, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;

            proto_tree_add_item(tree, hf_cliprdr_dwFlags, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;

            proto_tree_add_item(tree, hf_cliprdr_nPositionLow, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;

            proto_tree_add_item(tree, hf_cliprdr_nPositionHigh, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;

            proto_tree_add_item(tree, hf_cliprdr_cbRequested, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;

            proto_tree_add_item(tree, hf_cliprdr_clipDataId, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            break;
	case CB_FILECONTENTS_RESPONSE:
            proto_tree_add_item(tree, hf_cliprdr_streamId, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            break;
	case CB_LOCK_CLIPDATA:
	case CB_UNLOCK_CLIPDATA:
            proto_tree_add_item(tree, hf_cliprdr_clipDataId, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            break;
	default:
            break;
	}

	offset = nextOffset;
	return offset;
}

void proto_register_rdp_cliprdr(void) {
	static hf_register_info hf[] = {
		{ &hf_cliprdr_msgType,
		  { "OrderType", "rdp_cliprdr.ordertype",
		    FT_UINT16, BASE_HEX, VALS(rdp_cliprdr_order_vals), 0x0,
			NULL, HFILL }
		},
		{ &hf_cliprdr_msgFlags,
		  { "Flags", "rdp_cliprdr.msgflags",
			FT_UINT16, BASE_HEX, VALS(msgFlags_vals), 0x0,
			NULL, HFILL }
		},
		{ &hf_cliprdr_dataLen,
		  { "dataLen", "rdp_cliprdr.datalen",
			FT_UINT32, BASE_DEC, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_cliprdr_requestedFormatId,
		  { "requestedFormatId", "rdp_cliprdr.requestedformatid",
			FT_UINT32, BASE_HEX, VALS(knownFormats_vals), 0x0,
			NULL, HFILL }
		},
		{ &hf_cliprdr_clipDataId,
		  { "clipDataId", "rdp_cliprdr.clipdataid",
			FT_UINT32, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_cliprdr_streamId,
		  { "streamId", "rdp_cliprdr.streamid",
			FT_UINT32, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_cliprdr_lindex,
		  { "lindex", "rdp_cliprdr.lindex",
			FT_UINT32, BASE_DEC, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_cliprdr_dwFlags,
		  { "dwFlags", "rdp_cliprdr.dwflags",
			FT_UINT32, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_cliprdr_nPositionLow,
		  { "nPositionLow", "rdp_cliprdr.npositionlow",
			FT_UINT32, BASE_DEC, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_cliprdr_nPositionHigh,
		  { "nPositionHigh", "rdp_cliprdr.npositionhigh",
			FT_UINT32, BASE_DEC, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_cliprdr_cbRequested,
		  { "cbRequested", "rdp_cliprdr.cbrequested",
			FT_UINT32, BASE_DEC, NULL, 0x0,
			NULL, HFILL }
		},
	};

	static int *ett[] = {
		&ett_rdp_cliprdr,
	};

	proto_rdp_cliprdr = proto_register_protocol(PNAME, PSNAME, PFNAME);

	/* Register fields and subtrees */
	proto_register_field_array(proto_rdp_cliprdr, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));

	register_dissector("rdp_cliprdr", dissect_rdp_cliprdr, proto_rdp_cliprdr);
}

void proto_reg_handoff_rdp_cliprdr(void) {
}
