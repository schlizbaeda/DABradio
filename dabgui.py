#!/usr/bin/python3
# -*- coding: utf-8 -*-

#                 dabgui.py -- GUI frontend for dabd                 
#                   Copyright (C) 2019 schlizbäda
#                   mailto:himself@schlizbaeda.de
#
# dabgui.py is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License
# or any later version.
#             
# dabgui.py is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with dabgui.py. If not, see <http://www.gnu.org/licenses/>.
#
#
# Contributions:
# --------------
# Thanks to ...
#
# Installation:
# -------------
# t.b.d.
# 


import time
import signal       # implements correct behaviour for Ctrl-C in console
import sys          # contains sys.stdin und sys.stdout
import threading    # read from stdin within another thread

import tkinter      # the good old-fashioned tkinter GUI :-)
import tkinter.font


class dabgui(object):
    def __init__(self):
        self.gl_appName = 'dabgui.py'
        self.gl_appVer = '0.1'
        self.GL_VERBOSITY_NONE = 0      # really no additional output!
        self.GL_VERBOSITY_ERROR = 1     # no console output, only errors
        self.GL_VERBOSITY_NORMAL = 2    # print current action
        self.GL_VERBOSITY_DEBUG = 3     # print debug messages
        self.gl_verbosity = self.GL_VERBOSITY_ERROR
        
        self.GL_TERMINATION_MSG = '*MSG:  Press <ENTER> to terminate'
        self.GL_QUIT_FLAG_NONE = 0
        self.GL_QUIT_FLAG_NORMAL = 1
        self.GL_QUIT_FLAG_CTRL_C = 2
        self.GL_QUIT_FLAG_KILL = 4
        self.GL_QUIT_FLAG_DABD_TERMINATION = 8

        # init tkinter-GUI:
        self.gl_quit = self.GL_QUIT_FLAG_NONE
        self.create_gui()

    def create_gui(self):
        self.GUI = tkinter.Tk()
        self.GUI.title(self.gl_appName + ' V' + self.gl_appVer)
        
        # 1) Center main window on screen:
        w = int(self.GUI.winfo_screenwidth() * 3 / 4)
        h = int(self.GUI.winfo_screenheight() * 3 / 4)
        l = int((self.GUI.winfo_screenwidth() - w) / 2)   # centered
        t = int((self.GUI.winfo_screenheight() - h) / 2)  # centered
        geometry = str(w) + 'x' \
                   + str(h) + '+' \
                   + str(l) + '+' \
                   + str(t) # format string like '1280x720+0+0'
        self.GUI.geometry(geometry)
        fontsize = 12
        normalFont = tkinter.font.Font(family='DejaVuSans',
                                       weight='normal',
                                       size=fontsize)
        butFont =  tkinter.font.Font(family='DejaVuSansMono',
                                     weight='normal',
                                     size=fontsize)
        dispFont = tkinter.font.Font(family='DejaVuSansMono',
                                     weight='normal',
                                     size=fontsize)
        trackFont = tkinter.font.Font(family='DejaVuSansMono',
                                      weight='normal',
                                      size=3 * fontsize)
        
        # 2) Create menu:
        self.mnu_mainbar = tkinter.Menu(self.GUI, font = normalFont)
        # Menu bar "File"
        self.mnu_file = tkinter.Menu(self.mnu_mainbar,
                                     font=normalFont,
                                     tearoff=0)
        self.mnu_file.add_command(label='Quit',
                                  command=self.on_closing)
        # Menu bar "Help"
        self.mnu_help = tkinter.Menu(self.mnu_mainbar,
                                     font=normalFont,
                                     tearoff = 0)
        self.mnu_help.add_command(label='About',
                                  command=self.mnu_help_about_click)
        # Add sub menu bars in main menu:
        self.mnu_mainbar.add_cascade(label='File', menu=self.mnu_file)
        self.mnu_mainbar.add_cascade(label='Help', menu=self.mnu_help)
        # Show the complete menu structure:
        self.GUI.config(menu=self.mnu_mainbar)
        
        
        
        # 3) GUI layout: 
        # sashwidth=16:
        #   Enlarge divider's width
        #   for grabbing it easily on touch screens (with low accuracy):
        self.pw_mainpane = tkinter.PanedWindow(self.GUI,
                                               orient='horizontal',
                                               sashwidth=16)
        self.pw_mainpane.pack(fill='both', expand='yes')
        
        # Left side of pw_mainpane: List of available radio stations
        self.fra_radio_stations = tkinter.Frame(self.pw_mainpane)
        self.lbl_radio_stations = tkinter.Label(self.fra_radio_stations,
                                                font=normalFont,
                                                text='DAB+ Radio Stations')
        self.lbl_radio_stations.pack(side='top')
        #self.ysb_radio_stations = tkinter.Scrollbar(
        #                              self.fra_radio_stations,
        #                              orient=tkinter.VERTICAL)
        self.lst_radio_stations = tkinter.Listbox(
                                      self.fra_radio_stations,
                                      font=normalFont,
                                      selectmode=tkinter.BROWSE)
                                      #, xscrollcommand=None
                                      #,yscrollcommand=self.ysb_radio_stations.set)
        #self.ysb_radio_stations['command'] = self.lst_radio_stations.yview
        self.lst_radio_stations.pack(side='top',
                                     expand='yes',
                                     fill='both')
        self.pw_mainpane.add(self.fra_radio_stations)
        
        # Right side of pw_mainpane: "pane" for buttons and text box
        # sashwidth=0:
        #   prevent moving the pane divider because it can't be grabbed
        self.pw_communication = tkinter.PanedWindow(self.GUI,
                                                    orient='vertical',
                                                    sashwidth=0)
        if True:
            self.fra_buttons = tkinter.Frame(self.pw_communication)
            self.but_open = tkinter.Button(self.fra_buttons,
                                           font=butFont,
                                           width=4,
                                           text='Open')
            self.but_open.pack(side='left')
            self.but_close = tkinter.Button(self.fra_buttons,
                                            font=butFont,
                                            width=4,
                                            text='Close')
            self.but_close.pack(side='left')
            self.but_volume = tkinter.Button(self.fra_buttons,
                                             font=butFont,
                                             width=4,
                                             text='vol.9')
            self.but_volume.pack(side='left')
            self.but_stereo = tkinter.Button(self.fra_buttons,
                                             font=butFont,
                                             width=4,
                                             text='stereo')
            self.but_stereo.pack(side='left')
            self.but_play = tkinter.Button(self.fra_buttons,
                                           font=butFont,
                                           width=4,
                                           text='PLAY')
            self.but_play.pack(side='left')
            self.but_stop = tkinter.Button(self.fra_buttons,
                                           font=butFont,
                                           width=4,
                                           text='STOP')
            self.but_stop.pack(side='left')
            self.but_quit = tkinter.Button(self.fra_buttons,
                                           font=butFont,
                                           width=4,
                                           text='Quit')
            self.but_quit.pack(side='left')
            self.but_newline = tkinter.Button(self.fra_buttons,
                                              font=butFont,
                                              width=4,
                                              text='\\n')
            self.but_newline.pack(side='left')
            #self.lbl_separator1 = tkinter.Label(self.fra_buttons,
            #                                    font=normalFont)
            #self.lbl_separator1.pack()
            self.pw_communication.add(self.fra_buttons)
            
            self.fra_command = tkinter.Frame(self.pw_communication)
            self.but_command = tkinter.Button(self.fra_command,
                                              font=butFont,
                                              width=4,
                                              text='send')
            self.but_command.pack(side='left')
            self.ent_command = tkinter.Entry(self.fra_command,
                                             font=dispFont)
            self.ent_command.pack(side='left',
                                  expand='yes',
                                  fill='both')
            self.pw_communication.add(self.fra_command)
            
            #self.fra_display = tkinter.Frame(self.pw_communication,
            #                                 bg="yellow")
            #                                 #, relief=tkinter.SUNKEN)
            self.txt_display = tkinter.Text(self.pw_communication,
                                            font=dispFont)
            self.pw_communication.add(self.txt_display)

        self.pw_mainpane.add(self.pw_communication)
        
    def print_verbosity(self, verb, txt, sep=' ', end='\n'):
        if self.gl_verbosity >= verb:
            print(txt, sep = sep, end = end)

    def debug_print(self):
        # debug "sandbox" for any stuff
        self.print_verbosity(self.GL_VERBOSITY_ERROR, '')
        self.print_verbosity(self.GL_VERBOSITY_ERROR, 'DEB:')
        self.print_verbosity(self.GL_VERBOSITY_ERROR, 'F2 was pressed...')

    def mnu_help_about_click(self):
        def mnu_help_about_ok():
            about_box.destroy()

        self.print_verbosity(self.GL_VERBOSITY_NORMAL,
                             'cmd: mnu_help_about_click: ' \
                             + 'Show modal dialogue box')
        # GPL v3 - short version on file error:
        gpl_v3 = """\
                dabgui.py -- GUI frontend for dabd
                  Copyright (C) 2019 schlizbäda
                  mailto:himself@schlizbaeda.de

dabgui.py is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published
by the Free Software Foundation, either version 3 of the License
or any later version.
            
dabgui.py is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with dabgui.py. If not, see <http://www.gnu.org/licenses/>."""
        # read the original GPL v3 license text from file "COPYING":
        try:
            licfile = open('COPYING')
        except:
            licfile = None
        if not licfile is None:
            gpl_v3 = licfile.read()
            licfile.close()
        
        # modal about box
        # source: tkinter.unpythonic.net/wiki/ModalWindow
        about_box = tkinter.Toplevel()
        about_box.geometry('500x370')
        about_box.title('About ' + self.gl_appName \
                        + ' V' + self.gl_appVer)
        titleFont = tkinter.font.Font(family='DejaVuSans',
                                      weight='bold',
                                      size = 16)
        normalFont = tkinter.font.Font(family='DejaVuSans',
                                       weight='normal',
                                       size = 10)
        monospaceFont = tkinter.font.Font(family='Monospace',
                                          weight='normal',
                                          size = 8)
        lblTitle = tkinter.Label(about_box,
                                 text=self.gl_appName \
                                      + ' V' + self.gl_appVer,
                                 font=titleFont)
        lblTitle.pack()
        lblDesc1 = tkinter.Label(about_box,
                                 text="A tkinter GUI frontend for " \
                                      + "schlizbäda's dabd",
                                 font=normalFont)
        lblDesc1.pack(pady = 5)
        lblDesc2 = tkinter.Label(about_box,
                                 text='copyright (C) 2019 by schlizbäda',
                                 font=normalFont)
        lblDesc2.pack()
        lblDesc3 = tkinter.Label(about_box,
                                 text='mailto: himself@schlizbaeda.de',
                                 font=normalFont)
        lblDesc3.pack()
        lblLic = tkinter.Label(about_box,
                               text='  license (GNU GPL v3):',
                               font=normalFont)
        lblLic.pack(anchor = tkinter.W)
        txtLic = tkinter.Text(about_box,
                              height=16,
                              width=80,
                              font=monospaceFont)
        txtLic.pack()
        txtLic.insert('1.0', gpl_v3)
        butOK = tkinter.Button(about_box,
                               text='     OK     ',
                               font=normalFont,
                               command=mnu_help_about_ok)
        butOK.pack(pady=10)
        about_box.transient(self.GUI)   # 1st condition on modal box
        about_box.grab_set()            # 2nd condition on modal box
        self.GUI.wait_window(about_box) # 3rd condition on modal box


    ##### Function for reading from stdin called as a new thread:
    def stdin_thread(self, name):
        while not self.terminate_thread:
            lin = None
            try:
                lin = input()
            except EOFError:
                #print('EOFError')
                pass
            except BrokenPipeError:
                #print('BrokenPipeError')
                pass
            if lin is not None:
                self.txt_display.insert('end', lin + '\n')
                self.txt_display.see('end')
                term_msg_length = len(self.GL_TERMINATION_MSG)
                if lin[:term_msg_length] == self.GL_TERMINATION_MSG:
                    # termination message from dabd received!
                    self.gl_quit = self.GL_QUIT_FLAG_DABD_TERMINATION


    ##### Global event handlers of the GUI:
    def gui_keypress(self, event):
        # global "KeyPress" event for common control via keyboard
        #   F1: show about box
        #   F2: call debug outputs
        #  F10: opens the main menu: This seems to be a tkinter feature
        if event.char == event.keysym:
            msg = 'Normal Key %r' % event.char
            self.print_verbosity(self.GL_VERBOSITY_DEBUG,
                                 'DEB: gui_keypress: ' + msg)
        elif len(event.char) == 1:
            msg = 'Punctuation Key %r (%r)' % (event.keysym, event.char)
            self.print_verbosity(self.GL_VERBOSITY_DEBUG,
                                 'DEB: gui_keypress: ' + msg)
        else:
            msg = 'Special Key %r' % event.keysym
            self.print_verbosity(self.GL_VERBOSITY_DEBUG,
                                 'DEB: gui_keypress: ' + msg)
            if event.keysym == 'F1':
                self.mnu_help_about_click()
            elif event.keysym == 'F2':
                self.debug_print()
        
    def but_open_click(self, event):
        try:
            print('open') # command for testdab
            sys.stdout.flush()
        except BrokenPipeError:
            #print('BrokenPipeError')
            pass

    def but_close_click(self, event):
        try:
            print('close') # command for testdab
            sys.stdout.flush()
        except BrokenPipeError:
            #print('BrokenPipeError')
            pass

    def but_volume_click(self, event):
        try:
            print('set volume 9') # command for testdab
            sys.stdout.flush()
        except BrokenPipeError:
            #print('BrokenPipeError')
            pass

    def but_stereo_click(self, event):
        try:
            print('set stereo 1') # command for testdab
            sys.stdout.flush()
        except BrokenPipeError:
            #print('BrokenPipeError')
            pass

    def but_play_click(self, event):
        try:
            print('playstream 35') # command for testdab
            sys.stdout.flush()
        except BrokenPipeError:
            #print('BrokenPipeError')
            pass

    def but_stop_click(self, event):
        try:
            print('stopstream') # command for testdab
            sys.stdout.flush()
        except BrokenPipeError:
            #print('BrokenPipeError')
            pass

    def but_quit_click(self, event):
        try:
            print('quit') # command for testdab
            sys.stdout.flush()
        except BrokenPipeError:
            #print('BrokenPipeError')
            pass

    def but_newline_click(self, event):
        try:
            print() # command for testdab
            sys.stdout.flush()
        except BrokenPipeError:
            #print('BrokenPipeError')
            pass

    def but_command_click(self, event):
        try:
            print(self.ent_command.get()) # command for testdab
            sys.stdout.flush()
        except BrokenPipeError:
            #print('BrokenPipeError')
            pass

    def ent_command_keypress(self, event):
        # "KeyPress" event for entry widget ent_command:
        # check for <ENTER> key
        if event.char == event.keysym:
            # normal key
            pass
        elif len(event.char) == 1:
            # punctuation key (Return, Backspace, Tab, Escape, ...)
            if event.char == '\r': #and event.keysym == 'Return':
                # The <ENTER> key was pressed: submit command!
                self.but_command_click(None)
        else:
            # special key (cursor keys, ...)
            pass


    ##### Event handlers for Ctrl+C,                               #####
    #####                    "kill <process-ID>" and               #####
    #####                    terminate program ("Alt+F4")          #####
    def on_closing_cleanup(self):
        pass
    
    # Terminate program via WM_DELETE_WINDOW 
    # e.g. by pressing Alt+F4 in the GUI or clicking the red "X":
    def on_closing(self):
        self.on_closing_cleanup()
        # set termination value (1: program terminated normally):
        self.gl_quit = 1
        self.GUI.quit()

    # Program was aborted by Ctrl+C in the calling console terminal:
    def key_ctrl_c(self, signal, frame):
        self.on_closing_cleanup()
        # set termination value (2: Aborted with Ctrl-C):
        self.gl_quit = 2
        self.GUI.quit()

    # Program was aborted by "kill <process-ID>":
    def terminate_process(self, signal, frame):
        self.on_closing_cleanup()
        # set termination value (4: Terminated by "kill <process-ID>"):
        self.gl_quit = 4
        self.GUI.quit()
        
    # Periodic call for making Ctrl-C work:
    def do_nothing(self):
        if self.gl_quit == self.GL_QUIT_FLAG_DABD_TERMINATION:
            self.GUI.quit()
        else:
            self.GUI.after(200, self.do_nothing)


    def run(self):
        # Define event handlers for Ctrl+C, "kill <process-ID>"
        # and normal program termination (Alt+F4):
        self.GUI.protocol('WM_DELETE_WINDOW', self.on_closing) # normal exit
        signal.signal(signal.SIGINT, self.key_ctrl_c)          # aborted by Ctrl+C
        signal.signal(signal.SIGTERM, self.terminate_process)  # aborted by "kill <process-ID>"
        # Here it's possible to add further event handlers
        # for SIG... signals which cause a program termination.
        # These are almost all SIG... signals!
        # But schlizbäda didn't do that due to his laziness :-)
        self.GUI.after(200, self.do_nothing)
        
        # Bind event handler for keyboard hits:
        self.GUI.bind('<KeyPress>', self.gui_keypress)
        
        # Bind event handlers for buttons of the GUI:
        self.but_open.bind('<Button-1>', self.but_open_click)
        self.but_close.bind('<Button-1>', self.but_close_click)
        self.but_volume.bind('<Button-1>', self.but_volume_click)
        self.but_stereo.bind('<Button-1>', self.but_stereo_click)
        self.but_play.bind('<Button-1>', self.but_play_click)
        self.but_stop.bind('<Button-1>', self.but_stop_click)
        self.but_quit.bind('<Button-1>', self.but_quit_click)
        self.but_newline.bind('<Button-1>', self.but_newline_click)
        self.but_command.bind('<Button-1>', self.but_command_click)
        self.ent_command.bind('<KeyPress>', self.ent_command_keypress)
        
        # Start another thread for reading from stdin:
        self.terminate_thread = False
        thr_stdin = threading.Thread(target=self.stdin_thread,
                                     args=('stdin-thread',),
                                     daemon=True)
        thr_stdin.start()
        
        # Start tkinter mainloop:
        self.GUI.mainloop()
        if self.gl_quit & self.GL_QUIT_FLAG_DABD_TERMINATION:
            self.but_newline_click(None)  # don't send another 'quit' cmd!
        else:
            self.but_quit_click(None)     # ensure to quit ./dabd
        self.terminate_thread = True
        #thr_stdin.join()
        
        # Check and print termination status:
        if self.gl_quit & self.GL_QUIT_FLAG_NORMAL:
            self.print_verbosity(
                    self.GL_VERBOSITY_NORMAL,
                    self.gl_appName \
                    + ' terminated "normally".')
        elif self.gl_quit & self.GL_QUIT_FLAG_CTRL_C:
            self.print_verbosity(
                    self.GL_VERBOSITY_NORMAL,
                    self.gl_appName \
                    + ' aborted due to Ctrl+C in the calling terminal.')
        elif self.gl_quit & self.GL_QUIT_FLAG_KILL:
            self.print_verbosity(
                    self.GL_VERBOSITY_NORMAL,
                    self.gl_appName \
                    + ' was aborted by "kill <process-ID>".')
        elif self.gl_quit & self.GL_QUIT_FLAG_DABD_TERMINATION:
            self.print_verbosity(
                    self.GL_VERBOSITY_NORMAL,
                    self.gl_appName \
                    + ' was aborted by receiving "' \
                    + self.GL_TERMINATION_MSG \
                    + '" from stdin.')
        else:
            self.print_verbosity(
                    self.GL_VERBOSITY_NORMAL,
                    self.gl_appName \
                    + ' terminated in an unknown way: self.gl_quit=' \
                    + str(self.gl_quit))


if __name__ == '__main__':
    tkinter_app = dabgui().run()

#EOF
