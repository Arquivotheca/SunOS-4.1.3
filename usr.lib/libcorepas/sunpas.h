/*	@(#)sunpas.h 1.1 92/07/30 SMI	*/
/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */
function allocateraster(var rptr:rasttyp):integer; external;
function awaitanybutton(tim:integer;
				var buttonnum:integer):integer; external;
function awtbuttongetloc2(time:integer; locatornum:integer;
				var buttonnum:integer; var x:real;
				var y:real):integer; external;
function awtbuttongetval(time:integer; valnum:integer;
				var buttonnum:integer; var val:real):
				integer; external;
function awaitkeyboard(tim:integer;keynum:integer;var sptr:cct;
				var length:integer):integer; external;
function awaitpick(time:integer; picknum:integer;
				var segnam:integer; var pickid:integer)
				:integer; external;
function awaitstroke2(tim:integer;picknum:integer;asize:integer;var x:parr;
				var y:parr;numxy:integer):integer; external;
function beginbatchupdate:integer; external;
function closeretainseg:integer; external;
function closetempseg:integer; external;
function createretainseg(segname:integer):integer; external;
function createtempseg:integer; external;
function defcolorindices(var surfacename:vwsurf;
				i1:integer;i2:integer;
				var r:parr;var g:parr;var b:parr
				):integer; external;
function delallretainsegs:integer; external;
function delretainsegment(segname:integer):integer; external;
function deselectvwsurf(var surfacename:vwsurf
			):integer; external;
function endbatchupdate:integer; external;
function filetoraster(var rasfid:text; var rptr:rasttyp;
		      var map:cmap):integer; external;
function freeraster(var rptr:rasttyp):integer; external;
function getmousestate(devclass:integer;devnum:integer;
			var x:real;var y:real;var buttons:integer)
			:integer; external;
function getraster(var surfacename :vwsurf;
				xmin:real;xmax:real;ymin:real;ymax:real;
				xd:integer;yd:integer;var rptr:rasttyp):integer;
				external;
function getviewsurface(var surfacename:vwsurf):integer; external;
function initializecore(outputlevel:integer;
			inputlevel:integer;
			dimension:integer):integer; external;
function initializedevice(deviceclass:integer;
			devicenum:integer):integer; external;
function initializevwsurf(var surfacename:vwsurf; typ:integer
			):integer; external;
function inqcharjust(var chjust:integer):integer; external;
function inqcharpath2(var x:real;var y:real):integer; external;
function inqcharpath3(var x:real;var y:real;var z:real):integer; external;
function inqcharprecision(var chquality:integer):integer; external;
function inqcharsize(var width:real;var height:real):integer; external;
function inqcharspace(var space:real):integer; external;
function inqcharup2(var x:real;var y:real):integer; external;
function inqcharup3(var x:real;var y:real;var z:real):integer; external;
function inqcolorindices(var surfacename:vwsurf;
				i1:integer;i2:integer;
				var r:parr;var g:parr;var b:parr
				):integer; external;
function inqcurrpos2(var x:real;var y:real):integer; external;
function inqcurrpos3(var x:real;var y:real;var z:real):integer; external;
function inqdetectability(var detect:integer):integer; external;
function inqecho(devclass:integer;devnum:integer;
			var echotype:integer):integer; external;
function inqechoposition(devclass:integer;devnum:integer;
			var x:real;var y:real):integer; external;
function inqechosurface(devclass:integer;devnum:integer;
			var surfacename:vwsurf):integer; external;
function inqfillindex(var color:integer):integer; external;
function inqfont(var font:integer):integer; external;
function inqhighlighting(var highlight:integer):integer; external;
function inqimgtransform2(var sx:real; var sy:real;var a:real
				;var tx:real; var ty:real
				):integer; external;
function inqimgtransform3(var sx:real; var sy:real;var sz:real
				;var ax:real; var ay:real;var az:real
				;var tx:real; var ty:real;var tz:real
				):integer; external;
function inqimgxformtype(var segtype:integer):integer; external;
function inqimgtranslate2(var tx:real; var ty:real):integer; external;
function inqimgtranslate3(var tx:real; var ty:real;var tz:real
				):integer; external;
function inqinvcompmatrix(var iarray:ivarray):integer; external;
function inqkeyboard(keynum:integer;var bufsize:integer;var string:cct;
				var pos:integer):integer; external;
function inqlineindex(var color:integer):integer; external;
function inqlinestyle(var linestyle:integer):integer; external;
function inqlinewidth(var linewidth:real):integer; external;
function inqlocator2(locnum:integer;
			var x:real;var y:real):integer; external;
function inqmarkersymbol(var mark:integer):integer; external;
function inqndcspace2(var width:real;var height:real):integer; external;
function inqndcspace3(var width:real;var height:real;var 
			depth:real):integer; external;
function inqopenretainseg(var segname:integer):integer; external;
function inqopentempseg(var open:integer):integer; external;
function inqpen(var pen:integer):integer; external;
function inqpickid(var pick:integer):integer; external;
function inqpolyedgestyle(var pestyle:integer):integer; external;
function inqpolyintrstyle(var pistyle:integer):integer; external;
function inqprimattribs(var defprim:primattr):integer; external;
function inqprojection(var ptype:integer; var dx:real; var dy:real;
				var dz:real):integer; external;
function inqrasterop(var rastop:integer):integer; external;
function inqretainsegname(arraycnt:integer; var seglist:iarr;
				var segcnt:integer):integer; external;
function inqretainsegsurf(segname:integer; arraycnt:integer; var surflist:vwarr;
				var surfcnt:integer):integer; external;
function inqsegdetectable(segname:integer;var dtable:integer)
			:integer; external;
function inqseghighlight(segname:integer;var highlight:integer)
			:integer; external;
function inqsegimgxform2(segname:integer;var sx:real;var sy:real;
			var a:real;var tx:real;var ty:real
			):integer; external;
function inqsegimgxform3(segname:integer;var sx:real;var sy:real;
				var sz:real;var rx:real;var ry:real;
				var rz:real;var tx:real;var ty:real;var tz:real
):integer; external;
function inqsegimgxfrmtyp(segname:integer;var segtype:integer)
			:integer; external;
function inqsegimgxlate2(segname:integer;var tx:real;var ty:real)
			:integer; external;
function inqsegimgxlate3(segname:integer;var sx:real;var sy:real;
				var sz:real):integer; external;
function inqsegvisibility(segname:integer;var visible:integer):
			integer; external;
function inqstroke(strokenum:integer;var bufsize:integer;var 
			dist:real;var time:integer):integer; external;
function inqtextextent2(var string:cct;var dx:real; var dy:real
				):integer; external;
function inqtextextent3(var string:cct;var dx:real; var dy:real
				; var dz:real):integer; external;
function inqtextindex(var color:integer):integer; external;
function inqvaluator(valnum:integer;var init:real;var low:real;var high:real)
				:integer; external;
function inqviewdepth(var fdist:real;var bdist:real)
				:integer; external;
function inqviewplanedist(var vdist:real):integer; external;
function inqviewplanenorm(var dx:real; var dy:real;
				var dz:real):integer; external;
function inqviewrefpoint(var rx:real; var ry:real;
				var rz:real):integer; external;
function inqviewup2(var dx:real; var dy:real
				):integer; external;
function inqviewup3(var dx:real; var dy:real;
				var dz:real):integer; external;
function inqvwgcntrlparms(var wclip:integer;var fclip:integer;
				var bclip:integer;var typ:integer)
				:integer; external;
function inqviewingparams(var viewparm:vwprmtype):integer; external;
function inqviewport2(var xmin:real; var xmax:real;var ymin:real;var ymax:real
				):integer; external;
function inqviewport3(var xmin:real; var xmax:real;var ymin:real;var ymax:real
				;var zmin:real;var zmax:real)
				:integer; external;
function inqvisibility(var visible:integer)
			:integer; external;
function inqwindow(var umin:real; var umax:real;var vmin:real;var vmax:real
				):integer; external;
function inqworldmatrix2(var iarray:ivarray1):integer; external;
function inqworldmatrix3(var iarray:ivarray):integer; external;
function lineabs2(x:real;y:real):integer; external;
function lineabs3(x:real;y:real;z:real):integer; external;
function linerel2(x:real;y:real):integer; external;
function linerel3(x:real;y:real;z:real):integer; external;
function mapndctoworld2(ndx:real; ndy:real;
				var wldx:real; var wldy:real)
				:integer; external;
function mapndctoworld3(ndx:real; ndy:real; ndz:real;
				var wldx:real; var wldy:real
				; var wldz:real)
				:integer; external;
function mapworldtondc2(wldx:real; wldy:real;
				var ndx:real; var ndy:real)
				:integer; external;
function mapworldtondc3(wldx:real; wldy:real; wldz:real;
				var ndx:real; var ndy:real
				; var ndz:real
):integer; external;
function markerabs2(mx:real;my:real):integer; external;
function markerabs3(mx:real; my:real;mz:real):integer; external;
function markerrel2(dx:real;dy:real):integer; external;
function markerrel3(dx:real; dy:real;dz:real):integer; external;
function moveabs2(x:real;y:real):integer; external;
function moveabs3(x:real;y:real;z:real):integer; external;
function moverel2(x:real;y:real):integer; external;
function moverel3(x:real;y:real;z:real):integer; external;
function newframe:integer; external;
function pasloc(function f:integer):integer; external;
function polygonabs2(var xcoor:parr; var ycoor:parr;
		n:integer):integer; external;
function polygonabs3(var xcoor:parr; var ycoor:parr;var zcoor:parr;
		n:integer):integer; external;
function polygonrel2(var xcoor:parr; var ycoor:parr;
		n:integer):integer; external;
function polygonrel3(var xcoor:parr; var ycoor:parr;var zcoor:parr;
		n:integer):integer; external;
function polylineabs2(var xcoor:parr; var ycoor:parr;
		n:integer):integer; external;
function polylineabs3(var xcoor:parr; var ycoor:parr;var zcoor:parr;
		n:integer):integer; external;
function polylinerel2(var xcoor:parr;var ycoor:parr;
		n:integer):integer; external;
function polylinerel3(var xcoor:parr; var ycoor:parr;var zcoor:parr;
		n:integer):integer; external;
function polymarkerabs2(var xcoor:parr; var ycoor:parr;
		n:integer):integer; external;
function polymarkerabs3(var xcoor:parr; var ycoor:parr;var zcoor:parr;
		n:integer):integer; external;
function polymarkerrel2(var xcoor:parr; var ycoor:parr;
		n:integer):integer; external;
function polymarkerrel3(var xcoor:parr; var ycoor:parr;var zcoor:parr;
		n:integer):integer; external;
function printerror(var string:cct;error:integer):integer; external;
function putraster(var rptr:rasttyp):integer; external;
function puttext(var string:cct):integer; external;
function rastertofile(var rptr:rasttyp; var map:cmap; var rasfid:text;
		      n:integer):integer; external;
function renameretainseg(segname:integer;newname:integer):integer; external;
function reportrecenterr(var error:integer):integer; external;
function restoresegment(segname:integer; var fname:cct):integer; external;
function savesegment(segname:integer; var fname:cct):integer; external;
function selectvwsurf(var surfacename:vwsurf
			):integer; external;
function setbackclip(onoff:integer):integer; external;
function setcharjust(chjust:integer):integer; external;
function setcharpath2(dx:real; dy:real):integer; external;
function setcharpath3(dx:real; dy:real;dz:real):integer; external;
function setcharprecision(chquality:integer):integer; external;
function setcharsize(chwid:real;chht:real):integer; external;
function setcharspace(space:real):integer; external;
function setcharup2(dx:real; dy:real):integer; external;
function setcharup3(dx:real; dy:real;dz:real):integer; external;
function setcoordsystype(typ:integer):integer; external;
function setdetectability(detect:integer):integer; external;
function setdrag(drag:integer):integer; external;
function setecho(devclass:integer;devnum:integer;
			echotype:integer):integer; external;
function setechogroup(devclass:integer;var devarray:iarr;n:integer;
			echotype:integer):integer; external;
function setechoposition(devclass:integer;devnum:integer;
			x:real;y:real):integer; external;
function setechosurface(devclass:integer;devnum:integer;
		var surfacename:vwsurf):integer; external;
function setfillindex(color:integer):integer; external;
function setfont(font:integer):integer; external;
function setfrontclip(onoff:integer):integer; external;
function sethighlighting(highlight:integer):integer; external;
function setimgtransform2(sx:real; sy:real;a:real
				;tx:real; ty:real):integer; external;
function setimgtransform3(sx:real; sy:real;sz:real;
				ax:real; ay:real;az:real;
				tx:real; ty:real;tz:real)
				:integer; external;
function setimgxformtype(segtype:integer):integer; external;
function setimgtranslate2(tx:real; ty:real):integer; external;
function setimgtranslate3(tx:real; ty:real;tz:real):integer; external;
function setkeyboard(keynum:integer;bufsize:integer;var string:cct;
				pos:integer):integer; external;
function setlightdirect(dx:real; dy:real;dz:real
				):integer; external;
function setlineindex(color:integer):integer; external;
function setlinestyle(style:integer):integer; external;
function setlinewidth(width:real):integer; external;
function setlocator2(locnum:integer;x:real;y:real):integer; external;
function setmarkersymbol(mark:integer):integer; external;
function setndcspace2(width:real;height:real):integer; external;
function setndcspace3(width:real;height:real;depth:real)
				:integer; external;
function setoutputclip(onoff:integer):integer; external;
function setpen(pen:integer):integer; external;
function setpick(pickid:integer;aperture:real):integer; external;
function setpickid(pickid:integer):integer; external;
function setpolyedgestyle(pestyle:integer):integer; external;
function setpolyintrstyle(pistyle:integer):integer; external;
function setprimattribs(var defprim:primattr):integer; external;
function setprojection(ptype:integer;dx:real; dy:real;dz:real)
				:integer; external;
function setrasterop(rop:integer):integer; external;
function setsegdetectable(segname:integer; detectbl:integer)
				:integer; external;
function setseghighlight(segname:integer; highlight:integer)
				:integer; external;
function setsegimgxform2(segname:integer;sx:real; sy:real;a:real;
				tx:real;ty:real):integer; external;
function setsegimgxform3(segname:integer; sx:real;  sy:real;
				 sz:real; rx:real;  ry:real; rz:real
				; tx:real;  ty:real; tz:real
				):integer; external;
function setsegimgxlate2(segname:integer;tx:real; ty:real
				):integer; external;
function setsegimgxlate3(segname:integer;tx:real; ty:real;tz:real
				):integer; external;
function setsegvisibility(segname:integer;visible:integer):integer; external;
function setshadingparams(amb:real;dif:real;spec:real;flood:real;
				bump:real;hue:integer;style:integer
				):integer; external;
function setstroke(strokenum:integer;bufsize:integer;
			dist:real;time:integer)
			:integer; external;
function settextindex(color:integer):integer; external;
function setvaluator(valnum:integer;init:real;low:real;high:real)
			:integer; external;
function setvertexindices(var x:iarr;n:integer):integer; external;
function setvertexnormals(var xcoor:parr; var ycoor:parr;var zcoor:parr;
		n:integer):integer; external;
function setviewdepth(near:real;far:real):integer; external;
function setviewplanedist(dist:real):integer; external;
function setviewplanenorm(dx:real; dy:real;dz:real):integer; external;
function setviewrefpoint(x:real; y:real;z:real):integer; external;
function setviewup2(dx:real; dy:real):integer; external;
function setviewup3(dx:real; dy:real;dz:real):integer; external;
function setviewingparams(var viewparm:vwprmtype):integer; external;
function setviewport2(xmin:real;xmax:real;ymin:real;ymax:real):
			integer; external;
function setviewport3(xmin:real;xmax:real;ymin:real;ymax:real;zmin:real;
			zmax:real):integer; external;
function setvisibility(visibility:integer):integer; external;
function setwindow(umin:real;umax:real;vmin:real;vmax:real)
			:integer; external;
function setwindowclip(onoff:integer):integer; external;
function setworldmatrix2(var iarray:ivarray1):integer; external;
function setworldmatrix3(var iarray:ivarray):integer; external;
function setzbuffercut(var surfacename:vwsurf;var x:parr;var z:parr;n:integer)
			:integer; external;
function sizeraster(var surfacename:vwsurf;
				xmin:real;xmax:real;ymin:real;ymax:real;
				var rptr:rasttyp):integer; external;
function terminatecore:integer; external;
function terminatedevice(devclass:integer;devnum:integer):integer; external;
function terminatevwsurf(var surfacename:vwsurf):integer; external;
