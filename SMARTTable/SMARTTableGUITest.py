import Tkinter as tk
import ttk
import tkFont
#from VideoCapture import Device
from PIL import Image, ImageTk #, ImageEnhance, ImageDraw
import cv2
import sys, glob, os, time, math, copy, threading
import tkMessageBox
from tkMessageBox import *
#import serial
import pickle
#import pyflycap2
import numpy as np
import scipy
import matplotlib.pyplot as plt
import requests

#setting global constants for use in the code
number_of_cameras = 2
pic_width = 300
pic_height = 150

block_size = 10
grid = []
sequenceindex = 0
photocounter = 1
cam_index = 0

'''
#connects to the proper serial port using pyserial
ser = serial.Serial(
     port='COM4',
     baudrate=250000,
     parity=serial.PARITY_NONE,
     stopbits=serial.STOPBITS_ONE,
     bytesize=serial.EIGHTBITS,
     timeout=0,
     writeTimeout=0
)
#checks to see if the port is already opened
#possibly not needed
if not ser.isOpen():

    ser.isOpen()
'''
#secondary window for deletion of sequences
class SeqDeleteWindow(tk.Frame):

    #initializes window
    def __init__(self,master):

        #sets layering of window
        top=self.top= tk.Toplevel(master)
        #prevents interaction with main window
        top.grab_set()

        #creates label
        self.prompt = tk.Label(top, text = "Please select the sequence to delete.")
        self.prompt.pack(ipadx=50, ipady=10, fill='both', expand=True)

        #creates variables for:
        self.v = tk.StringVar() #optionmenu tracking
        self.value = None #sending the chosen sequence to the main window

        #utilizes the sequence list created in the main program
        #to store each sequence for selection

        #stores the first option for initialization
        self.v.set(app.sequence_list[0])
        #creates the optionmenu for choosing the sequence to delete
        self.options= tk.OptionMenu(top, self.v, *app.sequence_list)
        self.options.pack()
        #creates the button confirm the selection
        self.confirm_selection_button = tk.Button(top, text = "Confirm", command = self.cleanup)
        self.confirm_selection_button.pack()

    #returns the desired sequence for deletion
    def cleanup(self):

        #confirms the chosen sequence was correct with yes/no window
        response = tk.tkMessageBox.askyesno("Confirm", "Are you sure you want to delete '%s'?" %self.v.get())
        #if no, it returns to the prior window
        #to select a different sequence
        if not response:
            return
        #if yes,
        #saves the chosen value from the optionmenu
        #and destroys the window
        self.value = self.v.get()
        self.top.destroy()

#creates a window to set a sequence of multiple sequences to run
class MultiSeqWindow(tk.Frame):

    #initializes window
    def __init__(self,master):

        #sets layering of window
        top=self.top=tk.Toplevel(master)
        self.prompt = tk.Label(top, text = "Please select the sequence to add.")
        self.prompt.pack(ipadx=50, ipady=10, fill='both', expand=True)
        self.v = tk.StringVar()
        self.value = None

        #creates the listbox for storing list of sequences
        self.multiseq_listbox = tk.Listbox(top)
        self.multiseq_listbox.pack()

        #utilizes the sequence list created in the main program
        #to store each sequence for selection
        self.v.set(app.sequence_list[0])
        self.options= tk.OptionMenu(top, self.v, *app.sequence_list)
        self.options.pack()

        #creates add, delete, and confirm buttons
        self.add_seq_button = tk.Button(top, text = "Add", command = lambda: self.multiseq_listbox.insert(tk.END, self.v.get()))
        self.del_seq_button = tk.Button(top, text = "Delete", command = lambda: self.multiseq_listbox.delete(tk.ACTIVE))
        self.confirm_selection_button = tk.Button(top, text = "Confirm", command = self.cleanup)
        self.add_seq_button.pack()
        self.del_seq_button.pack()
        self.confirm_selection_button.pack()

    #returns the desired sequence for deletion
    def cleanup(self):

        #message box to ensure list is correct
        response = tk.tkMessageBox.askyesno("Confirm", "Is the above list correct?")
        if not response:
            return

        #stores the sequence list in the main window
        app.multi_seq_list = self.multiseq_listbox.get(0,tk.END)

        #handles closing of the window
        self.value = self.v.get()
        self.top.destroy()

#intial streaming window
#not currently used
class WebcamWindow(tk.Frame):

    #initializes window
    def __init__(self, master):

        tk.Frame.__init__(self)
        #sets layering of window
        top=self.top=tk.Toplevel(master)
        self.v = tk.StringVar()
        self.value = None
        self.webcam_update_boolean = True

        #Graphics window
        imageFrame = tk.LabelFrame(top, width=600, height=500)
        imageFrame.pack()

        #Capture video frames
        self.lmain = tk.Label(imageFrame, image = app.photo)
        self.lmain.image = app.photo
        self.lmain.pack()
        self.cap = cap = cv2.VideoCapture(0)

        self.quit_button = tk.Button(top, text = "Quit", command = self.cleanup)
        self.quit_button.pack()

        self.Show_Frame(cap)

    #shows one frame of the webcam
    #repeats every 10 milliseconds
    def Show_Frame(self, cap):

        global pic_width, pic_height
        if self.webcam_update_boolean:

            _, frame = cap.read()
            self.cv2image = frame
            self.cv2image = cv2.resize(self.cv2image,(pic_width, pic_height))
            self.livecv2image = cv2.cvtColor(self.cv2image, cv2.COLOR_BGR2RGBA)
            self.img = Image.fromarray(self.livecv2image)
            imgtk = ImageTk.PhotoImage(image=self.img)
            app.photo_label.config(image = imgtk)
            app.photo_label.image = imgtk
            app.photo_label.pack()
            self.after(10, lambda: self.Show_Frame(cap))

    #shows one frame without repeating
    #used for focusing
    def Update_Webcam(self, cap):

        _, frame = cap.read()
        self.cv2image = frame
        #self.cv2image = cv2.cvtColor(frame, cv2.COLOR_BGR2RGBA)
        self.img = Image.fromarray(self.cv2image)
        imgtk = ImageTk.PhotoImage(image=self.img)
        app.photo_label.config(image = imgtk)
        app.photo_label.image = imgtk
        app.photo_label.pack()

    #handles what happens on close
    def cleanup(self):
        self.webcam_update_boolean = False
        self.top.destroy()

#current working timer
#can be used to create multiple timers
#will probably create something to display the times left
class Thread_Timer(threading.Thread):

    def __init__(self, tk_root, sequence, total_time):

        #localization of sent variables
        self.root = tk_root
        self.sequence = sequence
        self.total_time = total_time

        #changes time to minutes
        self.total_time*=60


        #initializes and starts the timer
        threading.Thread.__init__(self)

        #allows timer to close when main program closes
        self.setDaemon(True)
        self.start()

    #begins on timer start
    def run(self):

        #loop to count the total time desired
        while self.total_time >0:
            self.total_time -= 1
            time.sleep(1)

        #if the user wants to repeat the sequence,
        #adds sequence back to the list
        if app.repeat_checkbutton.var.get() == 1:

            app.multi_seq_list_copy.append(self.sequence)

#main class
class Application(tk.Frame):

    #occurs as the application is initialized
    def __init__(self, master=None):

        '''
        self.lower_green = np.array([35,100,50])
        self.upper_green = np.array([75,255,255])
        '''

        #used for the spinbox validation
        self.vldt_ifnum_cmd = (master.register(self.ValidateIfNum),
                '%P', '%S', '%W')

        #initializes the window
        tk.Frame.__init__(self, master, padx = 5, pady = 5)
        self.parent = master

        #creation of variables
        self.sequence_list = []#all sequences
        self.multi_seq_list = []#store chosen set of sequences
        self.multi_seq_list_copy = []#stores copy of above
        self.sequence_photo_dict = {}#stores counters for pictures
        self.locations=[]#stores locations of grid clicks: not currently used

        #creates dictionary to store desired
        self.timer_dict = {}
        self.timer_dict= dict.fromkeys(['sequence1',
            'sequence2', 'sequence3', 'sequence4', 'sequence5'])

        self.running= False #prevents signals while moving
        self.readyToMoveBool=True #informs when motion is finished
        self.stopped_bool= False #detects "Stop" button
        self.webcam_update_boolean = True #allows for cancellation of webcam
        self.timer_created = False #ensures no duplicate creation of timers for one sequence
        self.next_sequence_bool = True #toggles looking for another seuquence in the list
        self.skip_new_timer = False #tracks if a sequence is already in the dictionary
        self.single_run_bool = False

        self.cap = cap = cv2.VideoCapture(0) #reference to camera stream

        #begins running functions
        self.Set_Limits()
        self.createWidgets()
        #self.Display_Location()
        self.Initialize_Sequences()
        self.Show_Frame(cap)

        #used to detect if the grid was clicked
        self.coord_plane.bind("<Button-1>",self.Grid_Clicked)

        #creates the initial frame of the window
        self.pack()

        #loads the photocounters from the pickle
        print os.getcwd()
        with open(".\\data\\txt\\sequences.pickle", "rb") as handle:
            self.sequence_photo_dict = pickle.load(handle)
        print self.sequence_photo_dict

    #obtains max positions of each axis from the text file
    #and sends to controller
    def Set_Limits(self):

        #dictionary to hold max values of table
        self.calib_coords = {}
        #read from the saved file where they are stored
        f = open(".\Table_Info3.0.txt", 'r')
        line = f.readline()
        lbl_holder, max_X, max_Y, max_Z = line.split()
        f.close()

        #saves the values into the dictionary
        self.calib_coords['X'] = int(max_X)
        self.calib_coords['Y'] = int(max_Y)
        self.calib_coords['Z'] = int(max_Z)

        #creates a copy of the values to write to the controller
        #prevents moving beyond the edges
        dcopy = copy.deepcopy(self.calib_coords)
        '''
        for key in dcopy:
            dcopy[key] = hex(dcopy[key])
            dcopy[key]  = dcopy[key][2:]
            dcopy[key] = self.Expand_to_Five_Digits(dcopy[key])
            self.Serial_Writing(key + "L"+dcopy[key])
        '''
    #creates the widgets on the app
    def createWidgets(self):

        #names the windows
        self.parent.title("SMART Table")

        #adds the menu to the window
        self.Add_Menu()

        #creates frame for the to hold the webcam stream
        self.photo_frame = tk.LabelFrame(self, text = "Display", width = pic_width, height = pic_height, padx = 5, pady = 5)
        self.photo_frame.grid(row=0, column=0)
        #initializes the frame with a saved picture
        camImg = Image.open(".\\data\\Pictures\\plant.jpg") #loads the image
        camImg = camImg.resize((pic_width,pic_height))
        self.photo = ImageTk.PhotoImage(camImg)
        self.photo_label = tk.Label(self.photo_frame)#, image = self.photo)
        self.photo_label.image = self.photo
        self.photo_label.pack()

        #self.unknown_home_button = Button(button_frame, text= "Home from Unknown Position", command = lambda: self.Home_From_Unknown_Position())

        #creates the frame to hold movement items
        direction_frame = tk.LabelFrame(self, text = "Directions", padx = 5, pady = 5)
        direction_frame.grid(row=2,column=1)
        #frame that holds the arrows for movements
        arrow_frame = tk.Frame(direction_frame, padx = 5, pady=5)
        arrow_frame.grid(row=0, column = 0)
        #frame that holds the increment changes for the movements
        increment_frame = tk.LabelFrame(direction_frame, text = "Step Size", padx = 5, pady = 5)
        increment_frame.grid(row=0, column = 1, rowspan = 2)
        #buttons for directional movement
        self.right_arrow_button = tk.Button(arrow_frame, text = "Right", height=2,width=7, command=lambda:self.Increment("X",1))
        self.left_arrow_button = tk.Button(arrow_frame, text ="Left",height=2,width=7, command=lambda:self.Increment("X",-1))
        self.back_arrow_button = tk.Button(arrow_frame, text = "Back",height=2,width=7, command=lambda:self.Increment("Y",1))
        self.forward_arrow_button = tk.Button(arrow_frame, text = "Ahead",height=2,width=7, command=lambda:self.Increment("Y",-1))
        self.up_arrow_button = tk.Button(arrow_frame, text = "Up",height=2,width=7, command=lambda:self.Increment("Z",1))
        self.down_arrow_button = tk.Button(arrow_frame, text = "Down",height=2,width=7, command=lambda:self.Increment("Z",-1))
        self.right_arrow_button.grid(row = 1, column = 2)
        self.left_arrow_button.grid(row=1,column=0)
        self.forward_arrow_button.grid(row=0,column=1)
        self.back_arrow_button.grid(row=1,column=1)
        self.up_arrow_button.grid(row=0,column=3)
        self.down_arrow_button.grid(row=1,column=3)

        #button to return to home (0,0,0)
        self.home_button = tk.Button(arrow_frame, text = "Home", command = lambda:self.Home())
        self.home_button.grid(row=1,column=4)
        #button to stop movement
        self.stop_button = tk.Button(arrow_frame, text = "Stop", command = self.On_Stop)
        self.stop_button.grid(row=0,column=4)

        #creates radiobuttons for increment size
        self.increments = tk.IntVar(value = 10000) #initializes as 10000
        self.increment_10k = tk.Radiobutton(increment_frame, text = "10000", variable = self.increments, value = 10000).grid(row = 0, column = 0, sticky = tk.W)
        self.increment_5k = tk.Radiobutton(increment_frame, text = "5000", variable = self.increments, value = 5000).grid(row = 1, column = 0, sticky = tk.W)
        self.increment_1k = tk.Radiobutton(increment_frame, text = "1000", variable = self.increments, value = 1000).grid(row = 2, column = 0, sticky = tk.W)
        self.increment_100 = tk.Radiobutton(increment_frame, text = "100", variable = self.increments, value = 100).grid(row = 3, column = 0, sticky = tk.W)
        self.increment_10 = tk.Radiobutton(increment_frame, text = "10", variable = self.increments, value = 10).grid(row = 4, column = 0, sticky = tk.W)
        self.increment_1 = tk.Radiobutton(increment_frame, text = "1", variable = self.increments, value = 1).grid(row = 5, column = 0, sticky = tk.W)

        #holds labels for positions within the direction frame
        coordinate_frame = tk.LabelFrame(direction_frame, text = "Coordinates", padx = 5, pady = 5)
        coordinate_frame.grid(row=1,column=0)

        #creates column labels
        self.current_label = tk.Label(coordinate_frame, text = "Current")
        self.desired_label = tk.Label(coordinate_frame, text = "Desired")
        #used to underline the column labels
        f = tkFont.Font(self.current_label, self.current_label.cget("font"))
        f.configure(underline = True)
        self.current_label.configure(font=f)
        g = tkFont.Font(self.desired_label, self.desired_label.cget("font"))
        g.configure(underline = True)
        self.desired_label.configure(font=g)
        #creates label for each axis current position and entrybox/spinbox for desired position
        self.x_coordinate_label = tk.Label(coordinate_frame,text="X-Coordinate:")
        self.x_coordinate_current = tk.Label(coordinate_frame, text = "self.x")
        self.x_coordinate_entry = tk.Entry(coordinate_frame)
        #self.x_coordinate_entry = Spinbox(coordinate_frame, from_=0, to=self.calib_coords['X'], validate = 'all', validatecommand = self.vldt_ifnum_cmd)
        self.y_coordinate_label = tk.Label(coordinate_frame,text="Y-Coordinate:")
        self.y_coordinate_current = tk.Label(coordinate_frame, text = "self.y")
        self.y_coordinate_entry = tk.Entry(coordinate_frame)
        #self.y_coordinate_entry = Spinbox(coordinate_frame, from_=0, to=self.calib_coords['Y'], validate = 'all', validatecommand = self.vldt_ifnum_cmd)
        self.z_coordinate_label = tk.Label(coordinate_frame,text="Z-Coordinate:")
        self.z_coordinate_current = tk.Label(coordinate_frame, text = "self.z")
        self.z_coordinate_entry = tk.Entry(coordinate_frame)
        #self.z_coordinate_entry = Spinbox(coordinate_frame, from_=0, to=self.calib_coords['Z'], validate = 'all', validatecommand = self.vldt_ifnum_cmd)

        #sends the arm to the desired position
        self.submit_coordinates = tk.Button(coordinate_frame,text="Submit new Coordinates",
            command =lambda: self.Move_By_Coordinates(self.x_coordinate_entry.get(),
                self.y_coordinate_entry.get(),
                self.z_coordinate_entry.get()))

        #places all of the labels
        self.current_label.grid(row=0,column=1)
        self.desired_label.grid(row=0,column=2)
        self.x_coordinate_label.grid(row=1,column=0)
        self.x_coordinate_current.grid(row=1,column=1)
        self.x_coordinate_entry.grid(row=1,column=2)
        self.y_coordinate_label.grid(row=2,column=0)
        self.y_coordinate_current.grid(row=2,column=1)
        self.y_coordinate_entry.grid(row=2,column=2)
        self.z_coordinate_label.grid(row=3,column=0)
        self.z_coordinate_current.grid(row=3,column=1)
        self.z_coordinate_entry.grid(row=3,column=2)
        self.submit_coordinates.grid(row=4,column=0, columnspan = 2)

        #frame for the grid of the table
        self.coord_plane_frame = tk.LabelFrame(self, text = "Location Map", padx = 5, pady = 5)
        self.coord_plane_frame.grid(row=1, column=0, rowspan = 2)

        #sets ratio of grid to fit the table
        grid_width = int(pic_width*0.75)
        grid_height = int(grid_width*1.8793)
        #creates a canvas to place the squares to create the grid
        self.coord_plane = tk.Canvas(self.coord_plane_frame,width = grid_width, height = grid_height, borderwidth = 0)
        self.coord_plane.pack()
        #creates the grid and stores some variables
        offset, self.block_size, self.grid, self.height, self.width = self.Grid(grid_width,grid_height)

        #previously used when converting the motor steps to smaller numbers
        #dropped for precision
        self.conversion_dict, self.max_dict = self.Grid_Conversion(self.coord_plane, self.calib_coords)

        #creates the frame for the sequences
        self.sequence_box_frame = tk.LabelFrame(self, text = "Sequences", padx = 5, pady = 5)
        self.sequence_box_frame.grid(row = 0, column = 1, rowspan = 2)
        self.full_sequence_frame = tk.Frame(self.sequence_box_frame, padx = 5, pady = 5)
        self.full_sequence_frame.grid(row=0, column = 0, columnspan = 2)
        self.new_sequence_frame = tk.Frame(self.full_sequence_frame, padx = 5, pady = 5)
        self.new_sequence_frame.grid(row=1,column=0, columnspan = 3, rowspan = 5)
        self.sequence_button_frame = tk.Frame(self.sequence_box_frame, padx=5, pady=5)
        self.sequence_button_frame.grid(row = 0, column = 2)

        #creates the listbox to hold the steps of each sequence
        #and the scrollbar
        self.sequence_scrollbar1 = tk.Scrollbar(self.new_sequence_frame, orient = tk.VERTICAL)
        self.sequence_lb = tk.Listbox(self.new_sequence_frame, yscrollcommand = self.sequence_scrollbar1.set)
        self.sequence_scrollbar1.config(command = self.sequence_lb.yview)
        self.sequence_scrollbar1.pack(side = tk.RIGHT, fill = tk.Y)
        self.sequence_lb.pack(side = tk.LEFT, fill = tk.BOTH, expand = 1)

        #creates the label to hold the name of the sequence
        self.sequence_name_label = tk.Label(self.full_sequence_frame, text = "")
        self.sequence_name_label.grid(row = 0, column = 0)
        #creates buttons within the sequence frame
        self.delete_step = tk.Button(self.sequence_button_frame, text = "Delete Step", command = lambda: self.Delete_Step())
        self.exit_seqCreation = tk.Button(self.sequence_button_frame, text = "Exit", command = lambda: self.Exit_SeqCreation())
        self.edit_sequence_button = tk.Button(self.sequence_button_frame, text = "Edit Sequence", command = lambda: self.Edit_Sequence())
        self.save_new_sequence = tk.Button(self.sequence_button_frame, text = "Save", command = lambda: self.Save_Sequence_to_Text())
        self.run_sequence_button = tk.Button(self.sequence_button_frame, text = "Run", command = lambda: self.Run_Sequence())

        #variable to hold the status of the repeat option
        self.repeat_tracker= tk.IntVar()
        #creates the checkbutton for repeating sequences
        self.repeat_checkbutton = tk.Checkbutton(self.sequence_button_frame, text = "Repeat Sequence", variable = self.repeat_tracker)
        #stores the value of the checkbutton
        self.repeat_checkbutton.var = self.repeat_tracker

        #creates options for adding
        #then stores the selection
        self.add_options = ["Add ...","Add Location","Add Wait","Add Take Picture"]
        self.add_option = tk.StringVar()
        self.add_option.set(self.add_options[0])
        #adds the proper response to the listbox
        self.add_option_menu = tk.OptionMenu(self.sequence_button_frame, self.add_option, *self.add_options,
         command = self.Option_Functions)

        #prevents interactions with the sequence items until desired
        self.Disable_Sequence_Items(True)
        #adds items to the sequence frame
        self.add_option_menu.grid(row = 0, column = 0)
        self.save_new_sequence.grid(row = 1, column = 0)
        self.delete_step.grid(row = 2, column = 0)
        self.exit_seqCreation.grid(row = 3, column = 0)
        self.edit_sequence_button.grid(row=4, column = 0)
        self.run_sequence_button.grid(row = 5, column = 0)
        self.repeat_checkbutton.grid(row = 6, column = 0)

        #creates frame for fluid system
        fluids_frame = tk.LabelFrame(self, text = "Fluids and Load Cells", padx = 5, pady = 5)
        fluids_frame.grid(row=0, column = 3, rowspan = 4)

        #creates buttons within fluid frame
        self.z1e1_button = tk.Button(fluids_frame, text="Zone 1: Emitter 1", command=self.z1e1)
        self.z1e1_button.grid(column=0,row=0,padx=5,pady=5)

        self.z1e2_button = tk.Button(fluids_frame, text="Zone 1: Emitter 2", command=self.z1e2)
        self.z1e2_button.grid(column=0,row=1,padx=5,pady=5)

        self.z1e3_button = tk.Button(fluids_frame, text="Zone 1: Emitter 3", command=self.z1e3)
        self.z1e3_button.grid(column=0,row=2,padx=5,pady=5)

        self.z1e4_button = tk.Button(fluids_frame, text="Zone 1: Emitter 4", command=self.z1e4)
        self.z1e4_button.grid(column=0,row=3,padx=5,pady=5)

        self.z2e1_button = tk.Button(fluids_frame, text="Zone 2: Emitter 1", command=self.z2e1)
        self.z2e1_button.grid(column=0,row=4,padx=5,pady=5)

        self.z2e2_button = tk.Button(fluids_frame, text="Zone 2: Emitter 2", command=self.z2e2)
        self.z2e2_button.grid(column=0,row=5,padx=5,pady=5)

        self.z2e3_button = tk.Button(fluids_frame, text="Zone 2: Emitter 3", command=self.z2e3)
        self.z2e3_button.grid(column=0,row=6,padx=5,pady=5)

        self.z2e4_button = tk.Button(fluids_frame, text="Zone 2: Emitter 4", command=self.z2e4)
        self.z2e4_button.grid(column=0,row=7,padx=5,pady=5)

        self.z3e1_button = tk.Button(fluids_frame, text="Zone 3: Emitter 1", command=self.z3e1)
        self.z3e1_button.grid(column=0,row=8,padx=5,pady=5)

        self.z3e2_button = tk.Button(fluids_frame, text="Zone 3: Emitter 2", command=self.z3e2)
        self.z3e2_button.grid(column=0,row=9,padx=5,pady=5)

        self.z3e3_button = tk.Button(fluids_frame, text="Zone 3: Emitter 3", command=self.z3e3)
        self.z3e3_button.grid(column=0,row=10,padx=5,pady=5)

        self.z3e4_button = tk.Button(fluids_frame, text="Zone 3: Emitter 4", command=self.z3e4)
        self.z3e4_button.grid(column=0,row=11,padx=5,pady=5)

        self.zero_button = tk.Button(fluids_frame, text="Zero Load Cells", command=self.zero_clicked)
        self.zero_button.grid(column=3,row=12,pady=5,sticky="e")

        self.send = tk.Button(fluids_frame, text="Submit Fluid Information", command=self.send, width=20)
        self.send.grid(column=0,row=12,pady=10,sticky="e",rowspan=2)

        #create labels within fluid frame
        self.time_label = tk.Label(fluids_frame, text="Time of Automatic Watering:")
        self.time_label.grid(column=3,row=0,sticky='e')

        self.labelz1e1 = tk.Label(fluids_frame, text="mL")
        self.labelz1e1.grid(column=1,row=0,sticky='e')

        self.labelz1e2 = tk.Label(fluids_frame, text="mL")
        self.labelz1e2.grid(column=1,row=1,sticky='e')

        self.labelz1e3 = tk.Label(fluids_frame, text="mL")
        self.labelz1e3.grid(column=1,row=2,sticky='e')

        self.labelz1e4 = tk.Label(fluids_frame, text="mL")
        self.labelz1e4.grid(column=1,row=3,sticky='e')

        self.labelz2e1 = tk.Label(fluids_frame, text="mL")
        self.labelz2e1.grid(column=1,row=4,sticky='e')

        self.labelz2e2 = tk.Label(fluids_frame, text="mL")
        self.labelz2e2.grid(column=1,row=5,sticky='e')

        self.labelz2e3 = tk.Label(fluids_frame, text="mL")
        self.labelz2e3.grid(column=1,row=6,sticky='e')

        self.labelz2e4 = tk.Label(fluids_frame, text="mL")
        self.labelz2e4.grid(column=1,row=7,sticky='e')

        self.labelz3e1 = tk.Label(fluids_frame, text="mL")
        self.labelz3e1.grid(column=1,row=8,sticky='e')

        self.labelz3e2 = tk.Label(fluids_frame, text="mL")
        self.labelz3e2.grid(column=1,row=9,sticky='e')

        self.labelz3e3 = tk.Label(fluids_frame, text="mL")
        self.labelz3e3.grid(column=1,row=10,sticky='e')

        self.labelz3e4 = tk.Label(fluids_frame, text="mL")
        self.labelz3e4.grid(column=1,row=11,sticky='e')

        self.combo = ttk.Combobox(fluids_frame, width=20, values=("1:00","2:00","3:00","4:00","5:00","6:00","7:00","8:00","9:00","10:00","11:00","12:00"))
        self.combo.grid(column=3,row=1,sticky='e')

        chk_state = tk.BooleanVar()
        chk_state.set(True)

        #place check button in fluid frame
        self.check = tk.Checkbutton(fluids_frame, text="Automatic Watering", var=chk_state)
        self.check.grid(column=3,row=2,sticky='e')

        #place entry boxes in fluid frame
        self.timing = tk.Entry(fluids_frame)

        self.entryz1e1 = tk.Entry(fluids_frame, width=15)
        self.entryz1e1.grid(column=1,row=0,sticky='w')

        self.entryz1e2 = tk.Entry(fluids_frame, width=15)
        self.entryz1e2.grid(column=1,row=1,sticky='w')

        self.entryz1e3 = tk.Entry(fluids_frame, width=15)
        self.entryz1e3.grid(column=1,row=2,sticky='w')

        self.entryz1e4 = tk.Entry(fluids_frame, width=15)
        self.entryz1e4.grid(column=1,row=3,sticky='w')

        self.entryz2e1 = tk.Entry(fluids_frame, width=15)
        self.entryz2e1.grid(column=1,row=4,sticky='w')

        self.entryz2e2 = tk.Entry(fluids_frame, width=15)
        self.entryz2e2.grid(column=1,row=5,sticky='w')

        self.entryz2e3 = tk.Entry(fluids_frame, width=15)
        self.entryz2e3.grid(column=1,row=6,sticky='w')

        self.entryz2e4 = tk.Entry(fluids_frame, width=15)
        self.entryz2e4.grid(column=1,row=7,sticky='w')

        self.entryz3e1 = tk.Entry(fluids_frame, width=15)
        self.entryz3e1.grid(column=1,row=8,sticky='w')

        self.entryz3e2 = tk.Entry(fluids_frame, width=15)
        self.entryz3e2.grid(column=1,row=9,sticky='w')

        self.entryz3e3 = tk.Entry(fluids_frame, width=15)
        self.entryz3e3.grid(column=1,row=10,sticky='w')

        self.entryz3e4 = tk.Entry(fluids_frame, width=15)
        self.entryz3e4.grid(column=1,row=11,sticky='w')

    def zero_clicked(self):
        msgBox = tk.tkMessageBox.askyesnocancel('Confirmation Window','Are you sure?')
        if msgBox=='yes':
            root.destroy()
        else:
            print("Canceled")

    def send(self):
        print("Sent")

    def zero(self):
        print("Zero")

    def z1e1(self):
        msgBox = tk.tkMessageBox.askyesnocancel('Confirmation Window','Start Watering Sequence?')
        if msgBox=='yes':
            ser1.open()
            ser1.write('z1e1')
            output = ser1.readline().decode().split('\r\n')
            time.sleep(10)
            ser1.close()
            root.destroy()
            #insert opening serial port and sending command to call proper arduino function
        else:
            print("Canceled")
        print("Watering Zone 1: Emitter 1")

    def z1e2(self):
        print("Watering Zone 1: Emitter 2")

    def z1e3(self):
        print("Watering Zone 1: Emitter 3")

    def z1e4(self):
        print("Watering Zone 1: Emitter 4")

    def z2e1(self):
        print("Watering Zone 2: Emitter 1")

    def z2e2(self):
        print("Watering Zone 2: Emitter 2")

    def z2e3(self):
        print("Watering Zone 2: Emitter 3")

    def z2e4(self):
        print("Watering Zone 2: Emitter 4")

    def z3e1(self):
        print("Watering Zone 3: Emitter 1")

    def z3e2(self):
        print("Watering Zone 3: Emitter 2")

    def z3e3(self):
        print("Watering Zone 3: Emitter 3")

    def z3e4(self):
        print("Watering Zone 3: Emitter 4")


        '''
        #label creation to be used in displaying time for sequences
        #need to fix/finish integration
        self.time_display_frame = LabelFrame(self, text = "Time Display", padx = 5, pady = 5)
        self.time_display_frame.grid(row = 0, column = 2, rowspan = 2)

        self.sequence1_label = Label(self.time_display_frame, text = "Sequence 1").grid(row = 0, column = 0)
        self.sequence2_label = Label(self.time_display_frame, text = "Sequence 2").grid(row = 1, column = 0)
        self.sequence3_label = Label(self.time_display_frame, text = "Sequence 3").grid(row = 2, column = 0)
        self.sequence4_label = Label(self.time_display_frame, text = "Sequence 4").grid(row = 3, column = 0)
        self.sequence5_label = Label(self.time_display_frame, text = "Sequence 5").grid(row = 4, column = 0)

        self.sequence1_time_label = Label(self.time_display_frame, text = "Sequence 1").grid(row = 0, column = 1)
        self.sequence2_time_label = Label(self.time_display_frame, text = "Sequence 2").grid(row = 1, column = 1)
        self.sequence3_time_label = Label(self.time_display_frame, text = "Sequence 3").grid(row = 2, column = 1)
        self.sequence4_time_label = Label(self.time_display_frame, text = "Sequence 4").grid(row = 3, column = 1)
        self.sequence5_time_label = Label(self.time_display_frame, text = "Sequence 5").grid(row = 4, column = 1)
        '''

    #creates the menu
    def Add_Menu(self):

        #creates the menu with titles
        self.menu = tk.Menu(root, tearoff = 0)
        root.config(menu=self.menu)
        #menu options created
        self.filemenu = tk.Menu(self.menu, tearoff = 0)
        self.sequencemenu = tk.Menu(self.menu, tearoff = 0)
        self.menu.add_cascade(label="File", menu = self.filemenu)
        self.menu.add_cascade(label="Sequences", menu = self.sequencemenu)

        #commands in 'File'
        self.filemenu.add_command(label="Check Camera", command= lambda: self.Test_Camera(pic_width,pic_height,self.photo_label))
        self.filemenu.add_command(label="Calibrate", command = lambda:self.Calibration())
        self.filemenu.add_command(label="Zero Photo Count", command = self.Initialize_Pickle)
        self.filemenu.add_command(label="GigE Test", command = self.GigETest)
        self.filemenu.add_separator()
        self.filemenu.add_command(label="Quit", command = self.quit)

        #commands in 'Sequences'
        self.sequencemenu.add_command(label="New Sequence", command = lambda: self.New_Sequence())
        self.sequencemenu.add_command(label="Delete Sequence", command = self.Delete_Sequence)
        self.sequencemenu.add_command(label="Multiple Sequences", command = self.Multiple_Sequences)
        self.sequencemenu.add_separator()

    #switches the widgets on or off depending on if a
    #sequence is running
    def Disable_Widgets(self, Disable):

        if Disable:
            state_1 = "disabled"
            state_2 = "active"
            entry_state = "disabled"
        else:
            state_1 = "active"
            state_2 = "disabled"
            entry_state = "normal"

        #states for extry boxes need a different name to enable
        self.x_coordinate_entry.configure(state = entry_state)
        self.y_coordinate_entry.configure(state = entry_state)
        self.z_coordinate_entry.configure(state = entry_state)

        self.home_button.configure(state = state_1)
        self.right_arrow_button.configure(state = state_1)
        self.left_arrow_button.configure(state = state_1)
        self.down_arrow_button.configure(state = state_1)
        self.up_arrow_button.configure(state = state_1)
        self.back_arrow_button.configure(state = state_1)
        self.forward_arrow_button.configure(state = state_1)
        self.submit_coordinates.configure(state = state_1)
        self.menu.entryconfig("File", state =state_1)
        self.menu.entryconfig("Sequences", state = state_1)
        self.stop_button.configure(state = state_2)

    #specifically toggles the widgets in the sequence box
    def Disable_Sequence_Items(self, Disable):

        if Disable:
            state_1 = "disabled"
            state_2 = "active"
            entry_state = "disabled"
        else:
            state_1 = "active"
            state_2 = "disabled"
            entry_state = "normal"

        self.add_option_menu.configure(state=state_1)
        self.delete_step.configure(state=state_1)
        self.exit_seqCreation.configure(state=state_1)
        self.save_new_sequence.configure(state=state_1)
        self.edit_sequence_button.configure(state= state_2)
        self.run_sequence_button.configure(state=state_2)
        #states for listboxes need a different name to enable
        self.sequence_lb.configure(state=entry_state)

    #controls looping of webcam display
    def Show_Frame(self, cap):

        #imports set size of image display
        global pic_width, pic_height

        #checks if webcam is still enabled
        if self.webcam_update_boolean:

            #reads frame from the webcam
            _, frame = cap.read()
            #stores and resizes image
            self.cv2image = frame
            self.cv2image = cv2.resize(self.cv2image,(pic_width, pic_height))
            #color conversion of the image
            self.livecv2image = cv2.cvtColor(self.cv2image, cv2.COLOR_BGR2RGBA)

            '''
            #converts the image to isolate most of the green pixel range
            hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
            self.mask = cv2.inRange(hsv,self.lower_green,self.upper_green)
            self.res = cv2.bitwise_and(frame,frame, mask = self.mask)
            #self.res = cv2.resize(self.res, (pic_width, pic_height))
            '''

            #converts to PIL image
            self.img = Image.fromarray(self.livecv2image)
            imgtk = ImageTk.PhotoImage(image=self.img)

            #displays image in the label
            self.photo_label.config(image = imgtk)
            self.photo_label.image = imgtk
            self.photo_label.pack()

            #updates every 10 ms
            self.after(10, lambda: self.Show_Frame(cap))

    #loads one new frame. used for obtaining clear image
    def Update_Webcam(self):

        #same as "Show_Frame" function
        #utilized in a loop to acquire a clear image

        _, frame = self.cap.read()
        self.cv2image = frame
        self.img = Image.fromarray(self.cv2image)
        imgtk = ImageTk.PhotoImage(image=self.img)
        self.photo_label.config(image = imgtk)
        self.photo_label.image = imgtk
        self.photo_label.pack()

    #allows for testing of multiple cameras to determine
    #desired camera
    #fix
    def Test_Camera(self,w,h, photo_label):

        global cam_index
        index = cam_index
        cam = cv2.VideoCapture(index)  # takes the picture
        #time.sleep(0.1)  # If you don't wait, the image will be dark
        s, img = cam.read() #checks if picture was taken and saves
        if s:    # frame captured without any errors
            cv2.imwrite("./data/Pictures/plant.jpg",img) #save image

            camImg = Image.open("./data/Pictures/plant.jpg") #loads the image
            camImg = camImg.resize((w,h))
            photo = ImageTk.PhotoImage(camImg)
            photo_label.config(image=photo)
            photo_label.image = photo
            photo_label.pack()

            #creating messagebox
            tk.Tk().wm_withdraw() #to hide the main window
            result = tk.tkMessageBox.askquestion('Verify','Is the proper camera displayed?',icon = 'warning')
            if result == "yes":
                #kicks out of function is right
                return int(index)
            else:
                #switches to alternate camera(s)
                cam_index = ((cam_index+1)%number_of_cameras)
                self.Test_Camera(w,h, photo_label)
        else:
            print( "Error in capture")
        return index

    #takes the picture using the selected camera
    def Take_Picture(self):

        #imports global variables
        global photocounter, cam_index, pic_height, pic_width

        #allows ordered image saving up to 100,000 folders
        fullphotocounter = self.Expand_to_Five_Digits(str(photocounter))

        #cleans up the name used for the sequence to create folder
        sequence_name=self.sequence_name_label.cget("text")
        trimmed_name=sequence_name.replace(" ","")

        #saves the path including the sequence name and the folder counter
        my_directory = "./data/Pictures/%s/Location%s" %(
            trimmed_name, str(fullphotocounter))

        #creates the folder if not aleady existing
        if not os.path.exists(my_directory):
            os.makedirs(my_directory)

        #allows ordered image saving up to 100,000 images in each folder
        iteration_number = self.Expand_to_Five_Digits(str(self.sequence_photo_dict[sequence_name]))
        '''
        #old naming procedure without embedded time
        #removed at same time as in picture labeling
        #updates the path to include the iteration number
        my_path = "C:/Python27/data/pictures/%s/Location%s/%s_%s.jpg" %(
            trimmed_name, str(fullphotocounter), str(fullphotocounter),
            iteration_number)
        '''

        my_path = "./data/Pictures/%s/Location%s/%s-%s_%s_%s.png" %(
            trimmed_name, str(fullphotocounter),
            time.strftime("%Y%m%d%H%M%S", time.localtime()),
            trimmed_name,
            str(fullphotocounter), iteration_number)

        self.after(1100)

        #loops five updates to increase clarity of the image captured
        i = 0
        while i != 5:
            i += 1
            self.Update_Webcam()

        #stores the image and ensures an image was captured
        img = self.cv2image
        if img.any() != None:
            s = True

        if s:    # frame captured without any errors

            '''
            #section of labeling specifics of images
            #can be expanded based on needs
            #removed to prevent blocking of any part of the image
            cv2.rectangle(img,(0,img.shape[0]-40),(200, img.shape[0]),(71,66,66),-1)
            cv2.putText(img, sequence_name,
                (10, img.shape[0]-30), cv2.FONT_HERSHEY_SIMPLEX, 0.35, (255,255,255), 1)
            cv2.putText(img, "%s - %s" %(str(fullphotocounter),iteration_number),
                (10, img.shape[0]-20), cv2.FONT_HERSHEY_SIMPLEX, 0.35, (255,255,255), 1)
            cv2.putText(img, time.strftime("%a, %d %b %Y %H:%M:%S ", time.localtime()),
                (10, img.shape[0]-10), cv2.FONT_HERSHEY_SIMPLEX, 0.35, (255,255,255), 1)
            '''

            cv2.imwrite(my_path,img) #save image
            #cv2.imwrite(png_path,img, [cv2.cv.CV_IMWRITE_PNG_COMPRESSION, 0])

            camImg = Image.open(my_path) #loads the image
            camImg = camImg.resize((pic_width,pic_height))
            photo = ImageTk.PhotoImage(camImg)

            #displays the image
            self.photo_label.config(image=photo)
            self.photo_label.image = photo
            self.photo_label.pack()

            photocounter +=1

    #triggers the RedEdge camera to capture an image
    #must be connected to the camera wifi to work
    def RedEdge_Capture(self):

        self.after(200)
        capture_params = { 'store_capture' : True, 'block' : True }
        capture_data = requests.post("http://192.168.10.254/capture", json=capture_params)
        self.after(200)

    #testing zone
    #does not do anything
    def GigETest(self):

        #self.RedEdge_Capture()

        #self.sequence_photo_dict['Tomato382.407.545'] = 0

        #attempt to get the point grey camera to capture
        #issue was receiving the feed
        #abandoned for now
        '''cc = pyflycap2.CameraContext()
        cc.rescan_bus()
        print(c.get_gige.cams())'''

    #moves to coordinates entered into entryboxes
    def Move_By_Coordinates(self,x,y,z):

        #kicks out if not integers in boxes
        try:
            self.x=int(x)
            self.y=int(y)
            self.z=int(z)

            #within bounds
            if self.x < self.calib_coords['X'] and self.y < self.calib_coords['Y'] and self.z < self.calib_coords['Z']:
                d={}
                d['X'] = self.x
                d['Y'] = self.y
                d['Z'] = self.z
                self.newLocDecDict = d

                #sends location to controller
                for key in self.newLocDecDict:
                    #print self.newLocDecDict[key]
                    self.Writing_New_Position_From_Dec(self.newLocDecDict[key], key)

                #moves when ready
                if self.readyToMoveBool:

                    self.readyToMoveBool=False
                    self.Move('M')

                #waits until ready to move
                else:
                    self.after(100, lambda: self.Move_By_Coordinates(x, y, z))

            #if larger than bounds, show error
            else:
                tk.tkMessageBox.showerror("Error", "Out Of Bounds.")

        #if not integer, show error
        except ValueError:
            tk.tkMessageBox.showerror('Error','Please use numbers as the coordinates.')

    #creates the grid for selecting a location
    def Grid(self,gridw,gridh):

        #initialization of grid
        width = int(gridw/block_size)
        height = int(gridh/block_size)
        offset = 2
        self.grid=[]

        #adds each box to the grid array for use in toggling
        for row in range(height):
            self.grid.append([])
            for column in range(width):
                x1 = column*block_size+offset
                y1 = row*block_size+offset
                x2 = x1+block_size
                y2 = y1+block_size
                self.coord_plane.create_rectangle(x1,y1,x2,y2, outline = "gray")
                self.grid[row].append(0)
        return offset, block_size, self.grid, height, width

    #move to approximate click location
    def Grid_Clicked(self, event):

        #determines location of click
        pos = event.x, event.y
        offset = 2
        column = (pos[0]-offset)//block_size
        row = (pos[1]-offset)//block_size

        #centers position within the box and displays message
        #asking if user wants to move
        roundedx = self.roundup(pos[0]-offset,block_size)+offset
        roundedy = self.roundup(pos[1]-offset,block_size)+offset
        roundedpos = (roundedx,roundedy)

        #FIX
        pixelpos = (roundedpos[0]*self.conversion_dict['X'],
            roundedpos[1]*self.conversion_dict['Y'])
        currpos = (self.x_coordinate_current.cget('text'),
            self.y_coordinate_current.cget('text'))
        newposmessage = "Are You Sure You Want to Move From\n %s to %s " %(repr(currpos), repr(pixelpos))

        #checks if within grid
        if row >=0 and column>=0:
            #cancels if writing failed
            try:

                print ("Click ", roundedpos, "Grid coordinates:",column+1,row+1, "Converted position:", pixelpos)
                result = tk.tkMessageBox.askyesno("New Position", newposmessage , icon='warning')
                #if yes, move to clicked location
                if result:
                    self.grid[row][column] = (self.grid[row][column] +1)%2
                    self.Writing_New_Position_From_Dec(pixelpos[0],'X')
                    self.Writing_New_Position_From_Dec(pixelpos[1],'Y')
                    self.Move('M')
                else:
                    print("Move Cancelled")
            except:
                print("fail")
                pass

        '''#SPECIFIC FOR STORING CHAIN OF GRID LOCATIONS....NOT NEEDED CURRENTLY
        if row>=0 and column>=0:
            try:
                self.grid[row][column] = (self.grid[row][column] +1)%2
                print ("Click ", roundedpos, "Grid coordinates:",column+1,row+1)
                if self.grid[row][column] ==1:
                    self.locations.append(roundedpos)
                else:
                    self.locations.remove(roundedpos)
                self.coord_plane.delete("all")
                self.coord_plane.create_rectangle(column*block_size+offset,
                    row*block_size+offset,
                    column*block_size+offset+block_size,
                    row*block_size+offset+block_size,
                    outline = "red")
            except:
                print "Wrong"
                pass
                '''

        #currently shows all of the locations clicked
        #need to switch to only most recent move
        for row in range(self.height):
            for column in range(self.width):
                color = "gray"
                if self.grid[row][column] == 1:
                    color= "black"
                x1 = column*block_size+offset
                y1 = row *block_size+offset
                x2 = x1+block_size
                y2 = y1+block_size
                self.coord_plane.create_rectangle(x1,y1,x2,y2, outline = color)

        '''#draws the lines between selected locations
        if len(self.locations)>1:
            self.coord_plane.create_line(self.locations)'''

    #rounds the click location to the center of the box clicked
    def roundup(self, x, n):
        return int(math.floor(x/n))*n +n/2
        pass

    #previously converted from a smaller scale grid system to pixels
    #not used because it resulted in minor precision lost
    def Grid_Conversion(self,coord_plane,calib_coords):

        maxes = {}
        maxz = 143
        maxes['X'] = coord_plane.winfo_reqwidth()
        maxes['Y'] = coord_plane.winfo_reqheight()
        maxes['Z'] = maxz
        print("Max X: " + str(coord_plane.winfo_reqwidth()))
        print("Max Y: " + str(coord_plane.winfo_reqheight()))
        print("Max Z: " + str(maxz))
        d = {}
        d['X'] = calib_coords['X']/coord_plane.winfo_reqwidth()
        d['Y'] = calib_coords['Y']/coord_plane.winfo_reqheight()
        d['Z'] = calib_coords['Z']/maxz
        return d, maxes

    #adjusts the location by the selected increment
    def Adjust_Location(self, value, direction):

        #determines direction of move
        jump = direction * self.increments.get()
        value +=jump

        #checks if move went beyond 0 and sets to 0
        if value < 0:
            tk.tkMessageBox.showwarning("Warning", "Movement went past home. Setting axis to 0.")
            value = 0
        return value

    #tests serial writing for troubleshooting
    def Serial_Test(self,entry):

        #checks if correct length of message given
        if len(entry) == 7:
            print(entry)
            #writes message to board
            ser.write(entry)
            out = ''
            time.sleep(0.1)
            #outputs everything written on board, including responses
            while ser.inWaiting()>0:
                out += ser.read(1)
            #prints to test
            if out != '':
                print(">>" + out)
                print(type(out))

            else:
                print("empty read")
        else:
            print("Incorrect size of input.")

        return out

    #writes command to controller
    #condensed form of 'Serial_Test'
    def Serial_Writing(self,entry):

        if len(entry) == 7:
            ser.write(entry)
            out =''
            time.sleep(0.1)
            while ser.inWaiting()>0:
                out += ser.read(1)
        else:
            return
        return out

    #changes output to integer format
    def Output_to_Integer(self,output):

        #turns the numerical output into a string to prevent
        #modification of the output
        output = repr(output)
        #removes the quotes on the ends to isolate the number
        output = output[1::]
        end = output.find(r"\r\n")
        output = output[:end]
        #returns number converted to integer for modification
        return int(output)

    #lengthens the new location in hex
    #to a five character location
    def Expand_to_Five_Digits(self,partHexLoc):

        #maintains the copy of the original numbers
        fullHexLoc = partHexLoc

        #concatenates "0"'s to the beginning of the number
        #to extend to a total of 5 digits
        x=0
        while x < 5-len(partHexLoc):
            fullHexLoc = "0" + fullHexLoc
            x+=1
        return fullHexLoc

    #takes the old location and changes to the desired location
    #then moves to the new location
    def Increment(self, axis, direction):

        #checks if table is moving
        if self.readyToMoveBool:

            self.readyToMoveBool=False

            #read the location of the desired axis
            entry = axis + 'R00000'
            #stores the location of the  axis
            response = self.Serial_Writing(entry)
            #exits the loop if nothing was returned
            if response == "":
                return
            #changes the location to integer format
            response = self.Output_to_Integer(response)
            #moves the location, stored in dec
            newLocDec = self.Adjust_Location(response, direction)
            #changes the new location from dec to hex
            self.Writing_New_Position_From_Dec(newLocDec,axis)

            #starts movement
            self.Move('M')

        #waits until table is not moving
        else:
            self.after(100, lambda: self.Increment(axis, direction))

    #tests if user input is a number
    def Is_Float(self, value):

        try:
            float(value)
            return True
        except ValueError:
            return False

    #sends command to move
    def Move(self, command):

        #sets boolean to true and allows stop button to be pressed again
        self.running = True
        self.Disable_Widgets(True)

        #command to move arm to new location or home
        ser.write('X' + str(command) + '00000')

        #initializes string for response
        s=""
        #delay for response
        self.Run_Until_Stop(s)

    #repeating function to check for stop command during motion
    #and determine responses from moves
    def Run_Until_Stop(self,s):

        #tries movement
        try:
            #if movement complete, do things
            if s == "MC":
                self.running = False
                print("Move Complete")
                self.Disable_Widgets(False)
                self.Display_Location()
                self.readyToMoveBool = True

            #if 'All set', means calibration is done
            #may eventually set limits to location
            if s == "AS":
                self.running = False
                self.readyToMoveBool = True
                self.Disable_Widgets(False)
                self.Display_Location()

            #reads one byte at a time
            bytestoread =ser.inWaiting()
            if bytestoread > 0:
                s = ser.read(bytestoread)

            #continues repeating until movement is done
            if self.running:

                self.after(10, lambda: self.Run_Until_Stop(s))

        except:
            print("exception")

    #stops the movement of the table
    def On_Stop(self):

        if self.running:

            #stops the user from sending an additional stop command
            self.stop_button.configure(state = "disabled")
            #boolean used to stop the loop
            self.running = False
            self.stopped_bool=True
            #writes the stop command
            self.Serial_Writing('XS00000')
            self.Disable_Widgets(False)
            self.Display_Location()

    #takes location in decimal and writes desired location
    def Writing_New_Position_From_Dec(self, newLocDec,axis):

        #converts location from dec to hex
        newLocHex = hex(newLocDec)
        #converts the hex into a 5 digit format
        newLocHex  = newLocHex[2:]
        finalLocHex = self.Expand_to_Five_Digits(newLocHex)
        #inserts letters for final serial formatting
        finalMove= axis + 'D' + finalLocHex
        #writes the move location
        response = self.Serial_Writing(finalMove)
        #exits if nothing was written
        if response == '':
            return

    #zeroes each axis
    def Home(self):
        self.Move('H')
        pass

    #displays current location of table
    def Display_Location(self):

        #stores the numerical position of the head
        self.x=self.Output_to_Integer(self.Serial_Writing('XR00000'))
        self.y=self.Output_to_Integer(self.Serial_Writing('YR00000'))
        self.z=self.Output_to_Integer(self.Serial_Writing('ZR00000'))

        #displays the location into the current position labels
        self.x_coordinate_current.config(text = self.x)
        self.y_coordinate_current.config(text = self.y)
        self.z_coordinate_current.config(text = self.z)

    #options for adding to sequence
    def Option_Functions(self, value):

        #uses the value of the selected option to determine
        #step to add to the sequence

        if value == "Add Location":
            self.Add_Location()
        elif value == "Add Wait":
            self.Add_Wait()
        elif value == "Add Take Picture":
            self.Add_Take_Picture()
        pass

    #takes the current position and adds to the sequence
    def Add_Location(self):

        #inserts x, y, z into the listbox

        self.sequence_lb.insert(tk.END, str(self.x) + ' , ' + str(self.y) + ' , ' + str(self.z) )
        pass

    #add a period of wait between steps to the sequence
    def Add_Wait(self):

        #asks desired time to wait in ms
        response = tk.simpledialog.askinteger("Time to Wait", "Time to Wait (ms):")
        #ensures answer is not empty or an integer
        if response != None:
            self.sequence_lb.insert(tk.END, "W" +str(response))

    #add a picture capture to the sequence
    def Add_Take_Picture(self):

        #inserts "TP" for "Take Picture"
        self.sequence_lb.insert(tk.END, "TP")
        pass

    #swaps widgets to enable sequence creation
    def New_Sequence(self):
        #enables creation of new sequence

        self.Disable_Sequence_Items(False)
        self.Disable_Widgets(False)
        self.stop_button.configure(state = "disabled")
        self.sequence_lb.delete(0,tk.END)
        self.sequence_name_label.configure(text = "")

    #creates an addition box to delete a sequence
    def Delete_Sequence(self):

        #creates window and waits for a response
        self.selection =SeqDeleteWindow(self.master)
        self.master.wait_window(self.selection.top)

        #ensures response is not blank
        if self.selection.value != None:

            #removes the selected sequence from all places
            self.sequencemenu.delete("%s" %self.selection.value)
            self.sequence_list.remove("%s" %self.selection.value)
            del self.sequence_photo_dict[self.selection.value]
            os.remove(".\\data\\txt\\Sequences\\%s.txt" %self.selection.value)

    #saves sequence to a text file under the name entered
    def Save_Sequence_to_Text(self):

        #creates boolean to track if the user would like to overwrite
        overwritebool= False

        #asks for sequence name
        savename = tk.simpledialog.askstring("Save Sequence", "Name this sequence.")

        #tests for certain responses
        #if blank, return
        #if a number, request a different name, because solo numbers cause issues
        if savename is None:
            return
        elif savename == "":
            tk.tkMessageBox.showerror("Error", "Please name the sequence.")
            return
        elif self.Is_Float(savename):
            tk.tkMessageBox.showerror("Error", "Name must contain at least one letter.")
            return
        else:

            #checks if sequence has been named the same as another
            for sequence in self.sequence_list:

                #checks all names to lowercase for easier checking
                if savename.lower() == sequence.lower():

                    #checks if the user would like to overwrite
                    result = tk.tkMessageBox.askyesno("Are you sure?", "There is already a file saved as %s."
                        "\n Would you like to overwrite it?" % sequence)

                    #if yes
                    #currently resets, assuming not continuation of old sequence
                    #will probably need to be changed for minor changes in sequence
                    if result:

                        #renames the folder to new case configuration
                        os.rename(".\\data\\txt\\Sequences\\%s.txt" %sequence,
                         ".\\data\\txt\\Sequences\\%s.txt" %savename)

                        #creates new sequence in menu
                        self.sequencemenu.entryconfigure(sequence, label = savename)
                        #adds sequence to list
                        index=self.sequence_list.index(sequence)
                        self.sequence_list[index] = savename
                        #initializes the iteration counter
                        self.sequence_photo_dict[savename] = 0
                        #uses the boolean to prevent adding an additional copy below
                        overwritebool = True
                    #if user does not want to overwrite
                    #requests a new name for the sequence
                    if not result:
                        self.Save_Sequence_to_Text()
                        return

            #if the new sequence name does not match an old one
            if not overwritebool:

                #adds new sequence to the menu
                self.sequencemenu.add_command(label = savename,
                 command = lambda: self.Open_Sequence(savename))

                #adds to list of sequences
                self.sequence_list.append(savename)
                #initializes iteration counter
                self.sequence_photo_dict[savename]=0

        fullname = ".\\data\\txt\\Sequences\\%s.txt" %savename
        f = open(fullname, "w+")

        items = self.sequence_lb.get(0,tk.END)
        for item in items:
            f.write(str(item) + "\r\n")
        f.close()
        self.sequence_name_label.config(text =savename)
        self.Disable_Sequence_Items(True)
        self.Disable_Widgets(False)

    #function to toggle widgets back
    def Exit_SeqCreation(self):

        #when exiting the sequence creation, toggle back
        self.Disable_Sequence_Items(True)
        self.Disable_Widgets(False)

    #deletes selected step in sequence
    def Delete_Step(self):

        #deletes the currently selected step in the sequence
        self.sequence_lb.delete(tk.ACTIVE)
        pass

    #loads the selected sequence from the saved listbox
    def Open_Sequence(self, name):

        #stores the desired name of the sequence to open
        savename = name
        self.sequence_name_label.config(text =savename)

        #creates the fullname directory to open the sequence
        fullname = ".\\data\\txt\\Sequences\\" + savename + ".txt"
        f = open(fullname, "r")

        #activates label, deletes current listbox stuff
        self.sequence_lb.configure(state = "normal")
        self.sequence_lb.delete(0,tk.END)

        #writes the lines into the listbox
        for line in f.readlines():
            if line.endswith('\r\n'):
                line = line[:-2]
            self.sequence_lb.insert(tk.END, line)

        #disables the listbox
        self.sequence_lb.configure(state = "disabled")

    #allows for multiple sequences to be run in sequence
    def Multiple_Sequences(self):

        #opens the window to select sequences for repetition
        #waits for response
        self.selection =MultiSeqWindow(self.master)
        self.master.wait_window(self.selection.top)

        #checks for a selection
        if self.selection.value != None:

            print(self.multi_seq_list)

            #stores the selected list
            #makes a copy for popping
            self.multi_seq_list= list(self.multi_seq_list)
            self.multi_seq_list_copy = list(self.multi_seq_list)
            self.Open_Sequence(self.multi_seq_list_copy.pop(0))
            self.Run_Sequence()

    #makes the lsitbox active for editing
    def Edit_Sequence(self):

        #checks if a sequence is loaded
        #if not, reports error
        if self.sequence_lb.size() == 0 and self.sequence_name_label.cget("text") == "":
            tk.tkMessageBox.showerror("Error", "Please load a sequence first.")
            return

        #toggles widgets
        self.Disable_Widgets(True)
        self.Disable_Sequence_Items(False)
        self.stop_button.configure(state = "disabled")

    #loads the sequences from the file for selection
    def Initialize_Sequences(self):

        #changes the directory to the "Sequences" directory
        os.chdir(".\\data\\txt\\Sequences\\")

        #saves all of the sequences in the directory to the menu
        #and the list of sequences
        for file in glob.glob("*.txt"):

            #cuts off the ".txt"
            file=file[:-4]
            self.sequencemenu.add_command(label = file, command = lambda file = file: self.Timer_Creation(file))
            self.sequence_list.append(file)

        os.chdir("..\\..\\..")

    #runs the steps. differentiates between the commands
    def Run_Sequence(self):

        global photocounter
        global sequenceindex

        #checks if the table is ready for the next sequence
        #loops if there is nothin else within the queue
        if self.next_sequence_bool and not self.multi_seq_list_copy:
            self.after(10, lambda: self.Run_Sequence())
            return

        #if there is a sequence queued
        #loads the sequence and stops checking for a new sequence
        elif self.next_sequence_bool and self.multi_seq_list_copy:
            self.Open_Sequence(self.multi_seq_list_copy.pop(0))
            self.next_sequence_bool = False

        #checks if listbox is empty
        if self.sequence_lb.size() == 0 and self.sequence_name_label.cget("text") == "":
            tk.tkMessageBox.showerror("Error", "Please load a sequence first.")
            return

        #checks if a timer was already created for this sequence
        #to add again for the next iteration
        #FIX: if a sequence is started while one is running with the
        #intent to not repeat, it will crash
        if not self.timer_created and self.repeat_checkbutton.var.get() == 1 and not self.single_run_bool:

            #prevents creation of multiple timers
            self.timer_created = True

            #obtains the key for the sequence being ran
            for key in self.timer_dict:
                if self.timer_dict[key] != None:
                    if self.timer_dict[key]['sequence'] == self.sequence_name_label.cget('text'):
                        sequence_track = key

            #creates the timer for the sequence
            #with the amount of run time desired
            Thread_Timer(self, self.timer_dict[sequence_track]['sequence'], self.timer_dict[sequence_track]['timeout'])

        #continues to run until there are no more steps in the sequence
        if sequenceindex < self.sequence_lb.size():

            #checks if manual stop was used
            if self.stopped_bool:
                return

            #only checks next step when not moving
            if not self.running:

                line = self.sequence_lb.get(sequenceindex)
                print(line)

                #should skip empty lines
                if line[0] == "":
                    pass

                #waits for time input
                elif line[0] == "W":
                    line = int(line[1:])
                    sequenceindex+=1
                    self.after(line)

                #takes a picture
                elif line[0:2] =="TP":

                    self.Take_Picture()
                    sequenceindex+=1

                elif line[0:2] == "RE":

                    self.RedEdge_Capture()
                    sequenceindex+=1

                #or moves to location
                else:
                    currx, curry, currz = line.split(" , ")
                    self.Move_By_Coordinates(currx, curry, currz)
                    sequenceindex += 1

            self.after(100, lambda: self.Run_Sequence())

        #when sequence is done
        else:

            photocounter = 1
            sequenceindex = 0
            self.Pickle_Track()
            self.next_sequence_bool = True
            self.timer_created = False
            self.Run_Sequence()

            #swapping to constantly check multi_seq_list_copy
            #for more sequences
            '''
            #checks if there are any more sequences to run
            if self.multi_seq_list_copy:
                self.Open_Sequence(self.multi_seq_list_copy.pop(0))
                self.Run_Sequence()

            #checks if repeat is desired
            elif self.repeat_checkbutton.var.get() == 1:

                #ensures full set of sequences is repeated if desired
                if self.multi_seq_list:

                    #creates a copy to remove steps one at a time
                    #and preserve the original
                    self.multi_seq_list_copy = list(self.multi_seq_list)

                    #opens the next sequence to run
                    self.Open_Sequence(self.multi_seq_list_copy.pop(0))

                #repeats if desired
                self.Run_Sequence()
            '''

            #constant checking for new sequences
            '''
            if not self.multi_seq_list_copy:
                self.sequence_lb.configure(state = "normal")
                self.sequence_lb.delete(0,END)
                self.Run_Sequence()
            '''

    #resets the folders for saving pictures
    def Initialize_Pickle(self):
        #needs to take the current self.sequence_list
        #and assure each sequence is keyed in
        #problem may be upon deletion of the sequence
        #if delete pickle key, recreation causes overlap of pics

        #change to individual sequence reset
        self.selection =SeqDeleteWindow(self.master)
        self.master.wait_window(self.selection.top)
        if self.selection.value != None:
            self.sequence_photo_dict['%s'%self.selection.value] = 0

    #increases the iteration number of the sequences
    def Pickle_Track(self):

        #increases the iteration by one
        self.sequence_photo_dict[self.sequence_name_label.cget("text")]+=1

        #stores the folder number of each sequence so no overwriting photos
        with open(".\\data\\txt\\sequences.pickle", "wb") as handle:
            pickle.dump(self.sequence_photo_dict, handle, protocol=pickle.HIGHEST_PROTOCOL)

    #calibrates the table
    #currently x and y
    def Calibration(self):

        #prompts for calibration confirmation
        calibration_string = ("Using this button will delete the current limits",
            " \n of the SMART Table and calibrate new ones.",
            " \n \n Are you sure you would like to re-calibrate?")
        result = tk.tkMessageBox.askyesno("Are you sure?", calibration_string , icon='warning')

        #runs calibration
        if result:
            self.running = True
            self.readyToMoveBool = False
            self.Disable_Widgets(True)
            self.Serial_Writing("XC00000")
            s= ""
            self.Run_Until_Stop(s)

    #will return to 0,0 when working
    def Home_From_Unknown_Position(self):

        #self.Serial_Writing("XU00000")
        pass

    #used to initialize timers for sequences
    def Timer_Creation(self, sequence):

        self.Open_Sequence(sequence)

        self.Repeat_Sequence()

    #creates a dictionary entry for the sequence to repeat "x" seconds
    def Repeat_Sequence(self):

        time = tk.simpledialog.askinteger("Time to Repeat", "Repeat after (min) [Enter '0' if no repetition]:")
        #ensures answer is not empty
        if time != None:

            if time == 0:
                #adds the sequence to the queue
                self.multi_seq_list_copy.append(self.sequence_name_label.cget("text"))
                self.single_run_bool = True
                return
            else:

                #checks each entry in dict for requested sequence
                for key in self.timer_dict:

                    #does not check the empty items
                    if self.timer_dict[key] != None:

                        #checks if duplicate timer request
                        if self.timer_dict[key]['sequence'] == self.sequence_name_label.cget("text"):

                            #asks if the user wants to replace the old time with the new time
                            response = tk.tkMessageBox.askyesno("Confirm", "A timeout of {0} minutes is already set for {1}."
                                "\n Would you like to replace it with {2} minutes?" .format(self.timer_dict[key]['timeout'],
                                    self.timer_dict[key]['sequence'], time))
                            #if yes
                            #changes time and skips creation of new timer
                            if response:
                                self.timer_dict[key]['timeout'] = time
                                self.skip_new_timer = True

                #if a new timer is needed (sequence has not already been timed)
                if not self.skip_new_timer:

                    #finds next available sequence
                    openkey = next(key for key in self.timer_dict if self.timer_dict[key] is None)
                    #enters the sequence and the amount of time to wait
                    self.timer_dict[openkey] = {
                     'sequence' : self.sequence_name_label.cget("text"),
                     'timeout' : time}

                    #adds the sequence to the queue
                    self.multi_seq_list_copy.append(self.timer_dict[openkey]['sequence'])

    #reject anything but numbers in the input
    #not currently used
    def ValidateIfNum(self, user_input, new_value, widget_name):

        valid = new_value == '' or new_value.isdigit()
        # now that we've ensured the input is only integers, range checking!
        if valid:
            # get minimum and maximum values of the widget to be validated
            minval = int(root.nametowidget(widget_name).config('from')[4])
            maxval = int(root.nametowidget(widget_name).config('to')[4])
            # check if it's in range
            if int(user_input) not in range (minval, maxval):
                valid = False
        if not valid:
            root.bell()
        return valid

root = tk.Tk()
app = Application(master=root)
try:
    app.master.iconbitmap(r'C:\\Python27\\data\\Logo.ico')
except:
    print 'Icon not found.'
app.mainloop()
app.cap.release()

try:
    #stores the folder number of each sequence so no overwriting photos
    with open(".\\data\\txt\\sequences.pickle", "wb") as handle:
        pickle.dump(app.sequence_photo_dict, handle, protocol=pickle.HIGHEST_PROTOCOL)
except FileNotFoundError:
    print 'File sequences.pickle is not found. Saving data to backup.pickle'
    pickle.dump(app.sequence_photo_dict, open('backup   .pickle','wb'))

root.destroy
