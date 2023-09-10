from tkinter import *
from tkinter import ttk
from winreg import *
import base64
import tkinter.messagebox
import pathlib

win = Tk()

win.title("USB Shield")

win.resizable(True,True)

w = win.winfo_reqwidth()
h = win.winfo_reqheight()
ws = win.winfo_screenwidth()
hs = win.winfo_screenheight()

final_w = (ws/2) - (w/2)
final_h = (hs/2) - (h/2)

win.geometry('+%d+%d' % (final_w, final_h))

style= ttk.Style()

settings_frame = Frame(win)
settings_frame.pack(side = TOP, fill = BOTH, expand=FALSE)

search_frame = Frame(win)
search_frame.pack(side = TOP, fill = BOTH, expand=FALSE)

bl_frame = Frame(win)
bl_frame.pack(side = LEFT, fill = BOTH, expand=TRUE)

wl_frame = Frame(win)
wl_frame.pack(side = RIGHT, fill = BOTH, expand=TRUE)

button_frame1 = Frame(bl_frame)
button_frame1.pack(side = BOTTOM)

button_frame2 = Frame(wl_frame)
button_frame2.pack(side = BOTTOM)

tree = ttk.Treeview(bl_frame, column=("c1", "c2"), show='headings')
tree2 = ttk.Treeview(wl_frame, column=("c1", "c2"),show='headings')

bl_path = "SOFTWARE\\USBShield\\Blacklist"
wl_path = "SOFTWARE\\USBShield\\Whitelist"
settings_path = "SOFTWARE\\USBShield"

def populate(path, tree):
   # JANGAN DIUBAH KECUALI PATH
   key = OpenKey(HKEY_LOCAL_MACHINE, path, 0, KEY_ALL_ACCESS)

   i = 0
   cond = True

   # Selected Row Value
   selected = "Instance1"

   while cond:
      try:
         x,y,z = EnumValue(key, i)

         tree.insert('', 'end', text=i+1, values=(y, x))
         
         i = i + 1
      except:
         cond = False

   CloseKey(key)

def refresh():

   for item in tree.get_children():
      tree.delete(item)

   populate(bl_path,tree)

   for item2 in tree2.get_children():
      tree2.delete(item2)

   populate(wl_path,tree2)

def settings_bar():

   source_image = PhotoImage(file = r"assets\\settings.png")
   button_image = source_image.subsample(20,20)

   settings_key = OpenKeyEx(HKEY_LOCAL_MACHINE, settings_path, 0, KEY_ALL_ACCESS)

   destination_address = QueryValueEx(settings_key, "dest_addr")[0]
   admin_email = QueryValueEx(settings_key, "adm_email")[0]
   admin_password = QueryValueEx(settings_key, "adm_password")[0]
   smtp_server = QueryValueEx(settings_key, "smtp_server")[0]

   #print(destination_address,admin_email,admin_password,smtp_server)

   if not all((destination_address, admin_email, admin_password, smtp_server)):
      settings_notes = ttk.Label(settings_frame, text= "Please configure your user settings before using the service!", font="Verdana 12 bold")
      settings_notes.pack(side="left", pady=10, fill="both")

   settings_btn = ttk.Button(settings_frame, text = "User Settings", image = button_image, compound=LEFT, command=user_settings)
   settings_btn.image = button_image
   settings_btn.pack(side="right", padx=5)

   settings_frame.rowconfigure(0, weight=1)
   settings_frame.columnconfigure(1, weight=1)

   if settings_key:
      CloseKey(settings_key)

def search_bar():

   search_label = ttk.Label(search_frame, text="Search:", font="Verdana 12 bold")
   search_label.pack(side="left", padx=5, pady=10, fill=X)

   search_entry = Entry(search_frame, width=20)
   search_entry.pack(side="left", padx=5, pady=10, ipady=3, fill=X, anchor=W)

   def search_bl():
      search_input = search_entry.get()
      search_result1 = []
      
      if search_input:
         for items in tree.get_children():
            if any(search_input.upper() in s for s in map(str.upper, tree.item(items)['values'])):
               print(tree.item(items)['values'])
               search_result1.append(items)

         if search_result1:
            tree.selection_set(search_result1)
            tkinter.messagebox.showinfo("Search completed", "Showing data searched in Blacklist.", parent=win)
         else:
            tkinter.messagebox.showinfo("Search completed", "No data found.", parent=win)
      else:
        tkinter.messagebox.showinfo("Error", "Input cannot be empty!", parent=win) 

   def search_wl():
      search_input = search_entry.get()
      search_result2 = []

      if search_input:
         for items2 in tree2.get_children():
            if any(search_input.upper() in s for s in map(str.upper, tree2.item(items2)['values'])):
               print(tree2.item(items2)['values'])
               search_result2.append(items2)

         if search_result2:
            tree2.selection_set(search_result2)
            tkinter.messagebox.showinfo("Search completed", "Showing data searched in Whitelist.", parent=win)
         else:
            tkinter.messagebox.showinfo("Search completed", "No data found.", parent=win)
      else:
        tkinter.messagebox.showinfo("Error", "Input cannot be empty!", parent=win)
      
   blsearch_button = ttk.Button(search_frame, text = "Search in Blacklist", compound=LEFT, command=search_bl)
   blsearch_button.pack(side="left", padx=5, pady=10, fill=X, anchor=W)

   wlsearch_button = ttk.Button(search_frame, text = "Search in Whitelist", compound=LEFT, command=search_wl)
   wlsearch_button.pack(side="left", padx=5, pady=10, fill=X, anchor=W)

   refresh_button = ttk.Button(search_frame, text = "Refresh Data", compound=LEFT, command=refresh)
   refresh_button.pack(side="left", padx=5, pady=10, fill=X, anchor=W)

   search_frame.rowconfigure(0, weight=1)
   search_frame.columnconfigure(1, weight=1)

def blacklist_table():
   title1 = ttk.Label(bl_frame, text="Blacklist", font="Verdana 10 bold")
   title1.pack(pady=10)

   tree.column("# 1", anchor=CENTER)
   tree.heading("# 1", text="Friendly Name")
   tree.column("# 2", anchor=CENTER)
   tree.heading("# 2", text="Instance ID")

   populate(bl_path, tree)
   
   yscrollbar = Scrollbar(bl_frame, orient="vertical", command=tree.yview)
   yscrollbar.pack(side="right", fill="y")

   tree.configure(yscrollcommand=yscrollbar.set)
   tree.pack(side="top", fill="both", expand=TRUE)
   
   # tree.bind('<Double-1>', lambda event, arg=tree: edit_blvalue(event, arg))

   move_btn1 = ttk.Button(button_frame1, text="Move to Whitelist", command=move_to_wl)
   move_btn1.pack(side="left", padx=5)

   del_btn1 = ttk.Button(button_frame1, text="Delete from Blacklist", command=delete_selection_bl)
   del_btn1.pack(side="left", padx=5)

   rename_btn1 = ttk.Button(button_frame1, text="Rename Value", command=edit_blvalue)
   rename_btn1.pack(side="left", padx=5)

   bl_frame.rowconfigure(0, weight=1)
   bl_frame.columnconfigure(1, weight=1)

def whitelist_table():
   title2 = ttk.Label(wl_frame, text="Whitelist", font="Verdana 10 bold")
   title2.pack(pady=10)

   tree2.column("# 1", anchor=CENTER)
   tree2.heading("# 1", text="Friendly Name")
   tree2.column("# 2", anchor=CENTER)
   tree2.heading("# 2", text="Instance ID")

   populate(wl_path, tree2)
   
   yscrollbar2 = Scrollbar(wl_frame, orient="vertical", command=tree.yview)
   yscrollbar2.pack(side="right", fill="y")
   
   tree2.configure(yscrollcommand=yscrollbar2.set)
   tree2.pack(side="top", fill="both", expand=TRUE)

   # tree2.bind('<Double-1>', lambda event, arg=tree2: edit_wlvalue(event, arg))

   move_btn2 = ttk.Button(button_frame2, text="Move to Blacklist", command=move_to_bl)
   move_btn2.pack(side="left", padx=5)

   del_btn2 = ttk.Button(button_frame2, text="Delete from Whitelist", command=delete_selection_wl)
   del_btn2.pack(side="left", padx=5)

   rename_btn2 = ttk.Button(button_frame2, text="Rename Value", command=edit_wlvalue)
   rename_btn2.pack(side="left", padx=5)

   wl_frame.rowconfigure(0, weight=1)
   wl_frame.columnconfigure(1, weight=1)


def delete_selection_bl():
   key = OpenKeyEx(HKEY_LOCAL_MACHINE, bl_path, 0, KEY_ALL_ACCESS)

   item_delete1 = tree.selection()
   bl_item = [tree.item(i, 'values') for i in item_delete1]

   for i, row in enumerate(bl_item, 1):
      # print(row[1])
      delete_bl_value = DeleteValue(key, row[1])

   for items in item_delete1:
      # print(items)
      tree.delete(items)
   
   if key:
      CloseKey(key)

   if item_delete1:
      tkinter.messagebox.showinfo("Delete Successful.", "You have successfully deleted the data from Blacklist.", parent=win)
   else:
      tkinter.messagebox.showinfo("Error.", "No data to be deleted.", parent=win)


def delete_selection_wl():
   key = OpenKeyEx(HKEY_LOCAL_MACHINE, wl_path, 0, KEY_ALL_ACCESS)

   item_delete2 = tree2.selection()
   wl_item = [tree2.item(i, 'values') for i in item_delete2]

   for i, row in enumerate(wl_item, 1):
      # print(row[1])
      delete_wl_value = DeleteValue(key, row[1])

   for items in item_delete2:
      # print(items)
      tree2.delete(items)
   
   if key:
      CloseKey(key)

   if item_delete2:
      tkinter.messagebox.showinfo("Delete Successful.", "You have successfully deleted the data from Whitelist.", parent=win)
   else:
      tkinter.messagebox.showinfo("Error.", "No data to be deleted.", parent=win)

def move_to_wl():
   bl_key = OpenKeyEx(HKEY_LOCAL_MACHINE, bl_path, 0, KEY_ALL_ACCESS)
   wl_key = OpenKeyEx(HKEY_LOCAL_MACHINE, wl_path, 0, KEY_ALL_ACCESS)

   selected_item1 = tree.selection()
   rows_item1 = [tree.item(i, 'values') for i in selected_item1]
   # Insert to next table
   for i, row in enumerate(rows_item1, 1):
      tree2.insert('', 'end', text="6", values=(row))
      # print(row[0], row[1])
      SetValueEx(wl_key, row[1], 0, REG_SZ, row[0])
      delete_bl_value = DeleteValue(bl_key, row[1])
   # Delete
   for items in selected_item1:
      # print(items)
      tree.delete(items)
   
   if bl_key:
      CloseKey(bl_key)
   if wl_key:
      CloseKey(wl_key)

   if selected_item1:
      tkinter.messagebox.showinfo("Move Successful.", "You have successfully moved the data to Whitelist.", parent=win)
   else:
      tkinter.messagebox.showinfo("Error.", "No data to be moved.", parent=win)


def move_to_bl():
   bl_key = OpenKeyEx(HKEY_LOCAL_MACHINE, bl_path, 0, KEY_ALL_ACCESS)
   wl_key = OpenKeyEx(HKEY_LOCAL_MACHINE, wl_path, 0, KEY_ALL_ACCESS)

   selected_item2 = tree2.selection()
   rows_item2 = [tree2.item(i, 'values') for i in selected_item2]
   # Insert to next table
   for i, row in enumerate(rows_item2, 1):
      tree.insert('', 'end', text="6", values=(row))
      # print(row[0], row[1])
      SetValueEx(bl_key, row[1], 0, REG_SZ, row[0])
      delete_bl_value = DeleteValue(wl_key, row[1])
   # Delete
   for items in selected_item2:
      # print(items)
      tree2.delete(items)

   if selected_item2:
      tkinter.messagebox.showinfo("Move Successful.", "You have successfully moved the data to Blacklist.", parent=win)
   else:
      tkinter.messagebox.showinfo("Error.", "No data to be moved.", parent=win)

def edit_blvalue():
   selected_item = tree.selection()
   value_edit = tree.item(selected_item, 'values')

   edit_popup = Toplevel(win)
   win_w = win.winfo_rootx()
   win_h = win.winfo_rooty()
   edit_popup.geometry('+%d+%d' % (win_w + 300, win_h + 100))
   edit_popup.title("Edit Values")
   edit_popup.grab_set()
   edit_popup.focus_set()
   edit_popup.attributes('-toolwindow', True)

   edit_popup.rowconfigure(0, weight=1)
   edit_popup.columnconfigure(0, weight=1)
   edit_popup.rowconfigure(5, weight=1)
   edit_popup.columnconfigure(5, weight=1)

   information = Label(edit_popup, text="Instance ID: " + value_edit[1])
   information.grid(row=1, column=1, columnspan=100, sticky = 'ew')

   friendly_name = Label(edit_popup, text="Friendly Name:")
   friendly_name.grid(row=2, column=1, sticky = 'w')
   
   name_input = Entry(edit_popup, width=50)
   name_input.grid(row=2, column=2, sticky = 'w')

   name_input.insert(0, value_edit[0])

   def confirm_edit(tree):
      new_name = name_input.get()
   
      tree.item(selected_item, values=(new_name, value_edit[1]))

      #Blacklist
      key = OpenKeyEx(HKEY_LOCAL_MACHINE, bl_path, 0, KEY_ALL_ACCESS)
      SetValueEx(key, value_edit[1], 0, REG_SZ, new_name)

      if key:
         CloseKey(key)

      edit_popup.destroy()

      tkinter.messagebox.showinfo("Edit Successful.", "You have successfully edited the data on Blacklist.", parent=win)

   cancel_button = Button(edit_popup, text="Cancel", command=edit_popup.destroy)
   cancel_button.grid(row=3, column=3, padx=5, pady=5)

   confirm_button = Button(edit_popup, text="Confirm", command=lambda:confirm_edit(tree))
   confirm_button.grid(row=3, column=4, padx=5, pady=5)

def edit_wlvalue():
   selected_item = tree2.selection()
   value_edit = tree2.item(selected_item, 'values')

   edit_popup = Toplevel(win)
   win_w = win.winfo_rootx()
   win_h = win.winfo_rooty()
   edit_popup.geometry('+%d+%d' % (win_w + 300, win_h + 100))
   edit_popup.title("Edit Values")
   edit_popup.grab_set()
   edit_popup.focus_set()
   edit_popup.attributes('-toolwindow', True)

   edit_popup.rowconfigure(0, weight=1)
   edit_popup.columnconfigure(0, weight=1)
   edit_popup.rowconfigure(5, weight=1)
   edit_popup.columnconfigure(5, weight=1)

   information = Label(edit_popup, text="Instance ID: " + value_edit[1])
   information.grid(row=1, column=1, columnspan=100, sticky = 'ew')

   friendly_name = Label(edit_popup, text="Friendly Name:")
   friendly_name.grid(row=2, column=1, sticky = 'w')
   
   name_input = Entry(edit_popup, width=50)
   name_input.grid(row=2, column=2, sticky = 'w')

   name_input.insert(0, value_edit[0])

   def confirm_edit(tree2):
      new_name = name_input.get()
   
      tree2.item(selected_item, values=(new_name, value_edit[1]))

      #Whitelist
      key = OpenKeyEx(HKEY_LOCAL_MACHINE, wl_path, 0, KEY_ALL_ACCESS)
      SetValueEx(key, value_edit[1], 0, REG_SZ, new_name)

      if key:
         CloseKey(key)

      edit_popup.destroy()

      tkinter.messagebox.showinfo("Edit Successful.", "You have successfully edited the data on Whitelist.", parent=win)

   cancel_button = Button(edit_popup, text="Cancel", command=edit_popup.destroy)
   cancel_button.grid(row=3, column=3, padx=5, pady=5)

   confirm_button = Button(edit_popup, text="Confirm", command=lambda:confirm_edit(tree2))
   confirm_button.grid(row=3, column=4, padx=5, pady=5)

def user_settings():

   settings_key = OpenKeyEx(HKEY_LOCAL_MACHINE, settings_path, 0, KEY_ALL_ACCESS)

   settings_popup = Toplevel(win)
   win_w = win.winfo_rootx()
   win_h = win.winfo_rooty()
   settings_popup.geometry('+%d+%d' % (win_w + 300, win_h + 100))
   settings_popup.title("User Settings")
   settings_popup.grab_set()
   settings_popup.focus_set()
   settings_popup.attributes('-toolwindow', True)

   settings_popup.rowconfigure(0, weight=1)
   settings_popup.columnconfigure(0, weight=1)

   settings_popup.rowconfigure(7, weight=1)
   settings_popup.columnconfigure(4, weight=1)  

   email_destination = Label(settings_popup, text="Email Destination: ")
   
   email_destination.grid(row = 1, column=1, sticky = 'w')

   destination_input = Entry(settings_popup, width=50)
   destination_input.insert(0, QueryValueEx(settings_key, "dest_addr")[0])
   destination_input.grid(row=1, column=2, sticky = 'w')

   admin_email = Label(settings_popup, text="Admin Email: ")
   admin_email.grid(row=2, column=1, sticky = 'w')

   email_input = Entry(settings_popup, width=50)
   email_input.insert(0, QueryValueEx(settings_key, "adm_email")[0])
   email_input.grid(row=2, column=2, sticky = 'w')

   admin_password = Label(settings_popup, text="Admin Password: ")
   admin_password.grid(row=3, column=1, sticky = 'w')

   password_input = Entry(settings_popup, show="*", width=50)
   stored_password = QueryValueEx(settings_key, "adm_password")[0]
   stored_bytes = stored_password.encode("ascii")
   decoded_password = base64.b64decode(stored_bytes)
   password_string = decoded_password.decode("ascii")
   password_input.insert(0, password_string)
   password_input.grid(row=3, column=2, sticky = 'w')

   email_server = Label(settings_popup, text="Email Server: ")
   email_server.grid(row=4, column=1, sticky = 'w')

   server_input = Entry(settings_popup, width=50)
   server_input.insert(0, QueryValueEx(settings_key, "smtp_server")[0])
   server_input.grid(row=4, column=2, sticky = 'w')

   server_note = Label(settings_popup, text= "Format for the email server is SMTP Server URL:Port, eg smtp.google.com:587")
   server_note.grid(row=5, column=1, sticky= 'w')

   def confirm_settings(settings_popup):

      settings_key = OpenKeyEx(HKEY_LOCAL_MACHINE, settings_path, 0, KEY_ALL_ACCESS)

      new_destination = destination_input.get()
      new_email = email_input.get()
      new_password = password_input.get()
      new_server = server_input.get()
   
      password_bytes = new_password.encode("ascii")
      base64_bytes = base64.b64encode(password_bytes)
      password = base64_bytes.decode("ascii")

      SetValueEx(settings_key, "dest_addr", 0, REG_SZ, new_destination)
      SetValueEx(settings_key, "adm_email", 0, REG_SZ, new_email)
      SetValueEx(settings_key, "adm_password", 0, REG_SZ, password)
      SetValueEx(settings_key, "smtp_server", 0, REG_SZ, new_server)

      settings_popup.destroy()

      tkinter.messagebox.showinfo("Configure Successful.", "You have successfully configured the user settings.", parent=win)

   def return_menu():
      settings_popup.destroy()

   cancel_button = Button(settings_popup, text="Cancel", command=lambda:return_menu())
   cancel_button.grid(row=6, column=1, padx=5, pady=5)

   confirm_button = Button(settings_popup, text="Confirm", command=lambda:confirm_settings(settings_popup))
   confirm_button.grid(row=6, column=2, padx=5, pady=5)

   if settings_key:
      CloseKey(settings_key)

settings_bar()
search_bar()
blacklist_table()
whitelist_table()

win.mainloop()