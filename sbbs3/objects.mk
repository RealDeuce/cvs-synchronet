# objects.mk

# Make 'include file' listing object files for SBBS.DLL

# $Id$

# LIBODIR, SLASH, and OFILE must be pre-defined

OBJS	=	$(LIBODIR)$(SLASH)ansiterm.$(OFILE)\
			$(LIBODIR)$(SLASH)answer.$(OFILE)\
			$(LIBODIR)$(SLASH)ars.$(OFILE)\
			$(LIBODIR)$(SLASH)atcodes.$(OFILE)\
			$(LIBODIR)$(SLASH)bat_xfer.$(OFILE)\
			$(LIBODIR)$(SLASH)base64.$(OFILE)\
			$(LIBODIR)$(SLASH)bulkmail.$(OFILE)\
			$(LIBODIR)$(SLASH)chat.$(OFILE)\
			$(LIBODIR)$(SLASH)chk_ar.$(OFILE)\
			$(LIBODIR)$(SLASH)con_hi.$(OFILE)\
			$(LIBODIR)$(SLASH)con_out.$(OFILE)\
			$(LIBODIR)$(SLASH)crc16.$(OFILE)\
			$(LIBODIR)$(SLASH)crc32.$(OFILE)\
			$(LIBODIR)$(SLASH)dat_rec.$(OFILE)\
			$(LIBODIR)$(SLASH)data.$(OFILE)\
			$(LIBODIR)$(SLASH)data_ovl.$(OFILE)\
			$(LIBODIR)$(SLASH)date_str.$(OFILE)\
			$(LIBODIR)$(SLASH)download.$(OFILE)\
			$(LIBODIR)$(SLASH)email.$(OFILE)\
			$(LIBODIR)$(SLASH)exec.$(OFILE)\
			$(LIBODIR)$(SLASH)execfile.$(OFILE)\
			$(LIBODIR)$(SLASH)execfunc.$(OFILE)\
			$(LIBODIR)$(SLASH)execmisc.$(OFILE)\
			$(LIBODIR)$(SLASH)execmsg.$(OFILE)\
			$(LIBODIR)$(SLASH)execnet.$(OFILE)\
			$(LIBODIR)$(SLASH)fido.$(OFILE)\
			$(LIBODIR)$(SLASH)file.$(OFILE)\
			$(LIBODIR)$(SLASH)filedat.$(OFILE)\
			$(LIBODIR)$(SLASH)getkey.$(OFILE)\
			$(LIBODIR)$(SLASH)getmail.$(OFILE)\
			$(LIBODIR)$(SLASH)getmsg.$(OFILE)\
			$(LIBODIR)$(SLASH)getnode.$(OFILE)\
			$(LIBODIR)$(SLASH)getstats.$(OFILE)\
			$(LIBODIR)$(SLASH)getstr.$(OFILE)\
			$(LIBODIR)$(SLASH)ini_file.$(OFILE)\
			$(LIBODIR)$(SLASH)inkey.$(OFILE)\
			$(LIBODIR)$(SLASH)ident.$(OFILE)\
			$(LIBODIR)$(SLASH)js_bbs.$(OFILE)\
			$(LIBODIR)$(SLASH)js_client.$(OFILE)\
			$(LIBODIR)$(SLASH)js_console.$(OFILE)\
			$(LIBODIR)$(SLASH)js_file.$(OFILE)\
			$(LIBODIR)$(SLASH)js_file_area.$(OFILE)\
			$(LIBODIR)$(SLASH)js_global.$(OFILE)\
			$(LIBODIR)$(SLASH)js_internal.$(OFILE)\
			$(LIBODIR)$(SLASH)js_msg_area.$(OFILE)\
			$(LIBODIR)$(SLASH)js_msgbase.$(OFILE)\
			$(LIBODIR)$(SLASH)js_server.$(OFILE)\
			$(LIBODIR)$(SLASH)js_socket.$(OFILE)\
			$(LIBODIR)$(SLASH)js_system.$(OFILE)\
			$(LIBODIR)$(SLASH)js_user.$(OFILE)\
			$(LIBODIR)$(SLASH)js_xtrn_area.$(OFILE)\
			$(LIBODIR)$(SLASH)listfile.$(OFILE)\
			$(LIBODIR)$(SLASH)load_cfg.$(OFILE)\
			$(LIBODIR)$(SLASH)logfile.$(OFILE)\
			$(LIBODIR)$(SLASH)login.$(OFILE)\
			$(LIBODIR)$(SLASH)logon.$(OFILE)\
			$(LIBODIR)$(SLASH)logout.$(OFILE)\
			$(LIBODIR)$(SLASH)lzh.$(OFILE)\
			$(LIBODIR)$(SLASH)mail.$(OFILE)\
			$(LIBODIR)$(SLASH)main.$(OFILE)\
			$(LIBODIR)$(SLASH)md5.$(OFILE)\
			$(LIBODIR)$(SLASH)msgdate.$(OFILE)\
			$(LIBODIR)$(SLASH)msgtoqwk.$(OFILE)\
			$(LIBODIR)$(SLASH)netmail.$(OFILE)\
			$(LIBODIR)$(SLASH)newuser.$(OFILE)\
			$(LIBODIR)$(SLASH)nopen.$(OFILE)\
			$(LIBODIR)$(SLASH)pack_qwk.$(OFILE)\
			$(LIBODIR)$(SLASH)pack_rep.$(OFILE)\
			$(LIBODIR)$(SLASH)postmsg.$(OFILE)\
			$(LIBODIR)$(SLASH)prntfile.$(OFILE)\
			$(LIBODIR)$(SLASH)putmsg.$(OFILE)\
			$(LIBODIR)$(SLASH)putnode.$(OFILE)\
			$(LIBODIR)$(SLASH)qwk.$(OFILE)\
			$(LIBODIR)$(SLASH)qwktomsg.$(OFILE)\
			$(LIBODIR)$(SLASH)readmail.$(OFILE)\
			$(LIBODIR)$(SLASH)readmsgs.$(OFILE)\
			$(LIBODIR)$(SLASH)ringbuf.$(OFILE)\
			$(LIBODIR)$(SLASH)scandirs.$(OFILE)\
			$(LIBODIR)$(SLASH)scansubs.$(OFILE)\
			$(LIBODIR)$(SLASH)scfglib1.$(OFILE)\
			$(LIBODIR)$(SLASH)scfglib2.$(OFILE)\
			$(LIBODIR)$(SLASH)scfgsave.$(OFILE)\
			$(LIBODIR)$(SLASH)smblib.$(OFILE)\
			$(LIBODIR)$(SLASH)smbtxt.$(OFILE)\
			$(LIBODIR)$(SLASH)sockopts.$(OFILE)\
			$(LIBODIR)$(SLASH)sortdir.$(OFILE)\
			$(LIBODIR)$(SLASH)str.$(OFILE)\
			$(LIBODIR)$(SLASH)str_util.$(OFILE)\
			$(LIBODIR)$(SLASH)telgate.$(OFILE)\
			$(LIBODIR)$(SLASH)telnet.$(OFILE)\
			$(LIBODIR)$(SLASH)text_sec.$(OFILE)\
			$(LIBODIR)$(SLASH)tmp_xfer.$(OFILE)\
			$(LIBODIR)$(SLASH)un_qwk.$(OFILE)\
			$(LIBODIR)$(SLASH)un_rep.$(OFILE)\
			$(LIBODIR)$(SLASH)upload.$(OFILE)\
			$(LIBODIR)$(SLASH)userdat.$(OFILE)\
			$(LIBODIR)$(SLASH)useredit.$(OFILE)\
			$(LIBODIR)$(SLASH)uucode.$(OFILE)\
			$(LIBODIR)$(SLASH)viewfile.$(OFILE)\
			$(LIBODIR)$(SLASH)writemsg.$(OFILE)\
			$(LIBODIR)$(SLASH)xtrn.$(OFILE)\
			$(LIBODIR)$(SLASH)xtrn_sec.$(OFILE)\
			$(LIBODIR)$(SLASH)yenc.$(OFILE)\
			$(LIBODIR)$(SLASH)genwrap.$(OFILE)\
			$(LIBODIR)$(SLASH)dirwrap.$(OFILE)\
			$(LIBODIR)$(SLASH)filewrap.$(OFILE)\
			$(LIBODIR)$(SLASH)threadwrap.$(OFILE)\
			$(LIBODIR)$(SLASH)semwrap.$(OFILE)\
			$(LIBODIR)$(SLASH)sockwrap.$(OFILE)\
			$(LIBODIR)$(SLASH)ver.$(OFILE)
