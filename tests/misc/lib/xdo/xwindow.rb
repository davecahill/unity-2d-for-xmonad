#Encoding: UTF-8
#This file is part of Xdo.
#Copyright © 2009, 2010 Marvin Gülker
#  Initia in potestate nostra sunt, de eventu fortuna iudicat.
#
# Modified by Gerry Boland <gerry dot boland at canonical dot com> 
require File.join(File.dirname(__FILE__), '_xdo')
require "open3"

module XDo
  
  #This class represents a window on the screen. Each window is uniquely identified by
  #an internal ID; before you can create a reference to a window (a XWindow object) you
  #have to obtain the internal ID of that window and pass it into XWindow.new.
  #Or you use the class methods of this class, notably XWindow.active_window.
  #
  #Via the XWindow object you get you can manipulate a window in serveral ways, e.g.
  #you can move or resize it. Some methods are not available on every window
  #manager: XWindow.active_window, XWindow.desktop_num, XWindow.desktop_num=, XWindow.desktop,
  #XWindow.desktop=, XWindow.from_active, #raise, #activate, #desktop, #desktop=.
  #Some of them may be available, some not. On my machine (an Ubuntu Lucid) for
  #example I can use active_window, desktop_num and #activate, but not #raise or #desktop=.
  #Those methods are tagged with the sentence "Part of the EWMH standard XY". Not all
  #parts of the EWMH standard are provided by every window manager.
  #
  #As of version 0.0.4 the way to search for window is about to change. The old version
  #where you passed a hash with symbols has been deprecated (and you get warnings
  #about this if you use it) in favor of passing those symbols as a rest argument. See
  #XWindow.search for more details.
  #
  #You should also be aware of the fact that XDo is about to support Regexp objects
  #in XWindow.search. In future versions (i.e. after the next minor release) strings
  #*always* mean an exact title/class/whatever match. For parts, you have to use
  #Regular Expressions. There is a culprit, though. +xdotool+ doesn't use Ruby's
  #Regular Expressions engine Oniguruma and expects C-style regexps. I don't know
  #about the differences - but if you're absolutely sure your window title matches
  #that wonderful three-line extended regexp and +xdotool+ doesn't find it, you
  #may email me at sutniuq@@gmx@net explaining which construct defeats +xdotool+.
  #I will then setup a list over time which states which constructs don't work.
  #
  #Be <i>very careful</i> with the methods that are part of the two desktop EWMH standards.
  #After I set the number of desktops and changed the current desktop, I had to reboot my
  #system to get the original configuration back. I don't know if I'm not using +xdotool+ correct,
  #but neither my library nor +xdotool+ itself could rescue my desktop settings. Btw, that's the
  #reason why it's not in XDo's unit tests (but it should work; at least in one way...).
  class XWindow
    #The internal ID of the window.
    attr_reader :id
    
    class << self
      
      #Checks if a window exists.
      #===Parameters
      #[+name+] The name of the window to look for. Either a string or a Regular Expression; however, there's no guaranty that +xdotool+ gets the regexp right. Simple ones should work, though.
      #[<tt>*opts</tt> (<tt>[:name, :class, :classname]</tt>) Search parameters. See XWindow.search.
      #===Return value
      #true or false.
      #===Example
      #  p XWindow.exists?("gedit") #=> true
      #  p XWindow.exists?(/^gedit/) #=> false
      #===Remarks
      #It may be a good idea to pass :onlyvisible as a search parameter.
      def exists?(name, *opts)
        if opts.first.kind_of?(Hash)
          warn("#{caller.first}: Deprecation Warning: Using a hash as further arguments is deprecated. Pass the symbols directly.")
          opts = opts.first.keys
        end
        
        !search(name, *opts).empty?
      end
      
      #Checks wheather the given ID exists or not.
      #===Parameters
      #[+id+] The ID to check for.
      #===Return value
      #true or false.
      #===Example
      #  p XWindow.id_exits?(29360674) #=> true
      #  p XWindow.id_exists?(123456) #=> false
      def id_exists?(id)
        return_code, out, err = XDo.execute("#{XDo::XWININFO} -id #{id}")
        return return_code == 0
      end
      
      #Waits for a window name to exist.
      #===Parameters
      #[+name+] The name of the window to look for. Either a string or a Regular Expression; however, there's no guaranty that +xdotool+ gets the regexp right. Simple ones should work, though.
      #[<tt>*opts</tt> (<tt>[:name, :class, :classname]</tt>) Search parameters. See XWindow.search.
      #===Return value
      #The ID of the newly appeared window.
      #===Example
      #  #Wait for a window with "gedit" somewhere in it's title:
      #  XDo::XWindow.wait_for_window("gedit")
      #  #Wait for a window that ends with "ends_with_this":
      #  XDo::XWindow.wait_for_window(/ends_with_this$/)
      #  #It's useful to combine this method with the Timeout module:
      #  require "timeout"
      #  Timeout.timeout(3){XDo::XWindow.wait_for_window("gedit")}
      #===Remarks
      #Returns immediately if the window does already exist.
      def wait_for_window(name, *opts)
        if opts.first.kind_of?(Hash)
          warn("#{caller.first}: Deprecation Warning: Using a hash as further arguments is deprecated. Pass the symbols directly.")
          opts = opts.first.keys
        end
        
        loop{break if exists?(name, *opts);sleep(0.5)}
        search(name, *opts).first
      end
      
      #Waits for a window to close.
      #===Parameters
      #[+name+] The name of the window to look for. Either a string or a Regular Expression; however, there's no guaranty that +xdotool+ gets the regexp right. Simple ones should work, though.
      #[<tt>*opts</tt> (<tt>[:name, :class, :classname]</tt>) Search parameters. See XWindow.search.
      #===Return value
      #nil.
      #===Example
      #  #Wait for a window with "gedit" somewhere in it's title
      #  XDo::XWindow.wait_for_close("gedit")
      #  #Waits for a window whose title ends with "ends_with_this":
      #  XDo::XWindow.wait_for_close(/ends_with_this$/)
      #  #It's quite useful to combine this method with the Timeout module:
      #  require "timeout"
      #  Timeout.timeout(3){XDo::XWindow.wait_for_close("gedit")}
      def wait_for_close(name, *opts)
        if opts.first.kind_of?(Hash)
          warn("#{caller.first}: Deprecation Warning: Using a hash as further arguments is deprecated. Pass the symbols directly.")
          opts = opts.first.keys
        end
        
        loop{break if !exists?(name, *opts);sleep(0.5)}
        nil
      end
      
      #Search for a window name to get the internal ID of a window.
      #===Parameters
      #[+str+] The name of the window to look for. Either a string or a Regular Expression; however, there's no guaranty that +xdotool+ gets the regexp right. Simple ones should work, though.
      #[<tt>*opts</tt> (<tt>[:name, :class, :classname]</tt>) Search parameters.
      #====Possible search parameters
      #Copied from the +xdotool+ manpage:
      #[class] Match against the window class.
      #[classname] Match against the window classname.
      #[name] Match against the window name. This is the same string that is displayed in the window titlebar.
      #[onlyvisible] Show only visible windows in the results. This means ones with map state IsViewable.
      #===Return value
      #An array containing the IDs of all found windows or an empty array
      #if none was found.
      #===Example
      #  #Look for every window with "gedit" in it's title, class or classname
      #  XDo::XWindow.search("gedit")
      #  #Look for every window whose title, class or classname ends with "SciTE"
      #  XDo::XWindow.search(/SciTE$/)
      #  #Explicitly only search the titles of visible windows
      #  XDo::XWindow.search("gedit", :name, :onlyvisible)
      def search(str, *opts)
        if opts.first.kind_of?(Hash)
          warn("#{caller.first}: Deprecation Warning: Using a hash as further arguments is deprecated. Pass the symbols directly.")
          opts = opts.first.keys
        end
        opts = [:name, :class, :classname] if opts.empty?
        
        #Allow Regular Expressions. Since I can't pass them directly to the command line,
        #I need to get their source. Otherwise we want an exact match, therefore the line
        #begin and line end anchors need to be set around the given string.
        str = str.source if str.kind_of?(Regexp)
        #TODO
        #The following is the new behaviour that will be activated with the next minor version.
        #See DEPRECATE.rdoc.
        #str = if str.kind_of?(Regexp)
          #str.source
        #else
          #"^#{str.to_str}$"
        #end
        
        cmd = "#{XDo::XDOTOOL} search "
        opts.each{|sym| cmd << "--#{sym} "}
        cmd << "'" << str << "'"
        #Don't handle errors since we want an empty array in case of an error
        return_code, out, err = XDo.execute(cmd)
        out.lines.to_a.collect{|l| l.strip.to_i}
      end
      
      #Returns the internal ID of the currently focused window.
      #===Parameters
      #[+notice_children+] (false) If true, childwindows are noticed and you may get a child window instead of a toplevel window.
      #===Return value
      #The internal ID of the found window.
      #===Raises
      #[XError] Error invoking +xdotool+.
      #===Example
      #  p XDo::XWindow.focused_window #=> 41943073
      #  p XDo::XWindow.focused_window(true) #=> 41943074
      #===Remarks
      #This method may find an invisible window, see active_window for a more reliable method.
      def focused_window(notice_children = false)
        return_code, out, err = XDo.execute("#{XDo::XDOTOOL} getwindowfocus #{notice_children ? "-f" : ""}")
        raise(XDo::XError, err) unless err.empty?
        out.to_i
      end
      
      #Returns the internal ID of the currently focused window.
      #===Return value
      #The ID of the found window.
      #===Raises
      #[XError] Error invoking +xdotool+.
      #===Example
      #  p XDo::XWindow.active_window #=> 41943073
      #===Remarks
      #This method is more reliable than #focused_window, but never finds an invisible window.
      #
      #Part of the EWMH standard ACTIVE_WINDOW.
      def active_window
        return_code, out, err = XDo.execute("#{XDo::XDOTOOL} getactivewindow")
        raise(XDo::XError, err) unless err.empty?
        out.to_i
      end
      
      #Set the number of working desktops.
      #===Parameters
      #[+num+] The number of desktops you want to exist.
      #===Return value
      #+num+.
      #===Raises
      #[XError] Error invoking +xdotool+.
      #===Example
      #  XDo::XWindow.desktop_num = 2
      #===Remarks
      #Although Ubuntu systems seem to have several desktops, that isn't completely true. An usual Ubuntu system only
      #has a single working desktop, on which Ubuntu sets up an arbitrary number of other "desktop views" (usually 4).
      #That's kind of cheating, but I have not yet find out why it is like that. Maybe it's due to the nice cube rotating effect?
      #That's the reason, why the desktop-related methods don't work with Ubuntu.
      #
      #Part of the EWMH standard WM_DESKTOP.
      def desktop_num=(num)
        return_code, out, err = XDo.execute("#{XDo::XDOTOOL} set_num_desktops #{num}")
        raise(XDo::XError, err) unless err.empty?
        num
      end
      
      #Get the number of working desktops.
      #===Return value
      #The number of desktops.
      #===Raises
      #[XError] Error invoking +xdotool+.
      #===Example
      #  p XDo::XWindow.desktop_num = 1
      #===Remarks
      #Although Ubuntu systems seem to have several desktops, that isn't completely true. An usual Ubuntu system only
      #has a single working desktop, on which Ubuntu sets up an arbitrary number of other "desktop views" (usually 4).
      #That's kind of cheating, but I have not yet find out why it is like that. Maybe it's due to the nice cube rotating effect?
      #That's the reason, why the desktop-related methods don't work with Ubuntu.
      #
      #Part of the EWMH standard WM_DESKTOP.
      def desktop_num
        return_code, out, err = XDo.execute("#{XDo::XDOTOOL} get_num_desktops")
        raise(XDo::XError, err) unless err.empty?
        out.to_i
      end
      
      #Change the view to desktop +num+.
      #===Parameters
      #[+num+] The 0-based index of the desktop you want to switch to.
      #===Return value
      #+num+.
      #===Raises
      #[XError] Error invoking +xdotool+.
      #===Example
      #  XDo::XWindow.desktop = 1
      #===Remarks
      #Although Ubuntu systems seem to have several desktops, that isn't completely true. An usual Ubuntu system only
      #has a single working desktop, on which Ubuntu sets up an arbitrary number of other "desktop views" (usually 4).
      #That's kind of cheating, but I have not yet find out why it is like that. Maybe it's due to the nice cube rotating effect?
      #That's the reason, why the desktop-related methods don't work with Ubuntu.
      #
      #Part of the EWMH standard CURRENT_DESKTOP.
      def desktop=(num)
        return_code, out, err = XDo.execute("#{XDo::XDOTOOL} set_desktop #{num}")
        raise(XDo::XError, err) unless err.empty?
        num
      end
      
      #Returns the number of the active desktop.
      #===Return value
      #The number of the currently shown desktop.
      #===Raises
      #[XError] Error invoking +xdotool+.
      #===Example
      #  p XDo::XWindow.desktop #=> 0
      #===Remarks
      #Although Ubuntu systems seem to have several desktops, that isn't completely true. An usual Ubuntu system only
      #has a single working desktop, on which Ubuntu sets up an arbitrary number of other "desktop views" (usually 4).
      #That's kind of cheating, but I have not yet find out why it is like that. Maybe it's due to the nice cube rotating effect?
      #That's the reason, why the desktop-related methods don't work with Ubuntu.
      #
      #Part of the EWMH standard CURRENT_DESKTOP.
      def desktop
        return_code, out, err = XDo.execute("#{XDo::XDOTOOL} get_desktop")
        raise(XDo::XError, err) unless err.empty?
        out.to_i
      end

      #Returns the geometry of the display.
      #===Return value
      #The width and height of the display as an array
      #===Raises
      #[XError] Error invoking +xdotool+.
      #===Example
      #  p XDo::XWindow.display_geometry #=> [1280,800]
      #  width, height = XDo::XWindow.display_geometry
      #===Remarks
      def display_geometry
        return_code, out, err = XDo.execute("#{XDo::XDOTOOL} getdisplaygeometry")
        raise(XDo::XError, err) unless err.empty?
        return out.split(/ /).first.to_i, out.split(/ /).last.to_i
      end

      #Creates a XWindow by calling search with the given parameters.
      #The window is created from the first ID found.
      #===Parameters
      #[+name+] The name of the window to look for. Either a string or a Regular Expression; however, there's no guaranty that +xdotool+ gets the regexp right. Simple ones should work, though.
      #[<tt>*opts</tt> (<tt>[:name, :class, :classname]</tt>) Search parameters. See XWindow.search.
      #===Return value
      #The created XWindow object.
      #===Raises
      #[XError] Error invoking +xdotool+.
      #===Example
      #  #Exact title/class/classname match
      #  xwin = XDo::XWindow.from_search("xwindow.rb - SciTE")
      #  #Part match via regexp
      #  xwin = XDo::XWindow.from_search(/SciTE/)
      #  #Part match via string - DEPRECATED.
      #  xwin = XDo::XWindow.from_search("SciTE")
      #  #Only search the window classes
      #  xwin = XDo::XWindow.from_search(/SciTE/, :class)
      def from_search(name, *opts)
        if opts.first.kind_of?(Hash)
          warn("#{caller.first}: Deprecation Warning: Using a hash as further arguments is deprecated. Pass the symbols directly.")
          opts = opts.first.keys
        end
        
        ids = search(name, *opts)
        raise(XDo::XError, "The window '#{name}' wasn't found!") if ids.empty?
        new(ids.first)
      end
      
      #_Deprecated_. Use XWindow.from_search or XWindow.from_title instead.
      def from_name(name, *opts)
        warn("#{caller.first}: Deprecation Warning: ::from_name is deprecated. Use ::from_search if you want the old behaviour with the ability to specify all search parameters, or ::from_title if you just want to look through the window titles.")
        from_search(name, *opts)
      end
      
      #Same as XWindow.from_search, but only looks for the window's titles to match.
      #===Parameters
      #[+title+] The title of the window to look for. Either a string or a Regular Expression; however, there's no guaranty that +xdotool+ gets the regexp right. Simple ones should work, though.
      #===Return value
      #A XWindow object made up from the first window ID found.
      #===Raises
      #[XError] Error invoking +xdotool+.
      #===Example
      #  #Exact string match
      #  xwin = XDo::XWindow.from_title("xwindow.rb - SciTE")
      #  #Part match via regexp
      #  xwin = XDo::XWindow.from_title(/SciTE/)
      #  #Part match via string - DEPRECATED.
      #  xwin = XDo::XWindow.from_title("SciTE")
      def from_title(title)
        from_search(title, :name)
      end
      
      #Creates a XWindow by calling XWindow.focused_window with the given parameter.
      #===Parameters
      #[+notice_children+] (false) If true, you may get a child window as the active window.
      #===Return value
      #The newly created XWindow objects.
      #===Example
      #  xwin = XDo::XWindow.from_focused
      #===Remarks
      #The XWindow.focused_window method is a bit dangerous, since it may
      #find an invisible window. Use XWindow.from_active if you don't want that.
      def from_focused(notice_childs = false)
        new(focused_window(notice_childs))
      end
      
      #Creates a XWindow by calling active_window.
      #===Return value
      #The newly created XWindow object.
      #===Example
      #  xwin = XDo::XWindow.from_active
      #===Remarks
      #This method does not find invisible nor child windows; if you want that,
      #you should take a look at XWindow.from_focused.
      def from_active
        new(active_window)
      end
      
      #Returns the ID of the root window.
      #===Return value
      #The ID of the root window.
      #===Example
      #  p XDo::XWindow.root_id #=> 346
      def root_id
        return_code, out, err = XDo.execute("#{XDo::XWININFO} -root")
        Kernel.raise(XDo::XError, err) unless err.empty?
        return out.lines.to_a[0].match(/Window id:(.*?)\(/)[1].strip.to_i
      end
      
      #Creates a XWindow refering to the root window.
      #===Return value
      #The newly created XWindow object.
      #===Example
      #  rwin = XDo::XWindow.from_root
      def from_root
        new(root_id)
      end
      
      #Creates a invalid XWindow.
      #===Return value
      #The newly created XWindow object.
      #===Example
      #  nwin = XDo::XWindow.from_null
      #===Remarks
      #The handle the returned XWindow object uses is zero and
      #therefore invalid. You can't call #move, #resize or other
      #methods on it, but it may be useful for unsetting focus.
      #See also the XWindow.unfocus method.
      def from_null
        new(0) #Zero never is a valid window ID. Even the root window has another ID.
      end
      
      #Unsets the input focus by setting it to the invalid
      #NULL window.
      #===Parameters
      #[+sync+] (true) If true, this method blocks until the input focus has been unset. 
      #===Return value
      #nil.
      #===Example
      #  win = XDo::XWindow.from_active
      #  win.focus
      #  XDo::XWindow.unfocus
      def unfocus(sync = true)
        from_null.focus(sync)
      end
      
      #Deprecated.
      def desktop_name=(name)
        warn("#{caller.first}: Deprecation warning: XWindow.desktop_name= doesn't do anything anymore.")
      end
      
      #Deprecated.
      def desktop_name
        warn("#{caller.first}: Deprecation warning: XWindow.desktop_name doesn't do anything anymore.")
        "x-nautilus-desktop"
      end
      
      #Deprecated. Just calls XWindow.unfocus internally.
      def focus_desktop
        warn("#{caller.first}: Deprecation warning: XWindow.focus_desktop is deprecated. Use XWindow.unfocus instead.")
        unfocus
      end
      alias activate_desktop focus_desktop
      
      #Minimize all windows (or restore, if already) by sending key combination 
      #either [CTRL]+[ALT]+[D] or [SUPER]+[D]. Check with window manager which one.
      #Available after requireing  "xdo/keyboard".
      #===Return value
      #Undefined.
      #===Raises
      #[NotImplementedError] You didn't require 'xdo/keyboard'.
      #[LoadError] Unable to get gconf value, check key set and gconftool-2 installed
      #===Example
      #  #Everything will be minimized:
      #  XDo::XWindow.toggle_minimize_all
      #  #And now we'll restore everything.
      #  XDo::XWindow.toggle_minimize_all
      def toggle_minimize_all
        raise(NotImplementedError, "You have to require 'xdo/keyboard' before you can use #{__method__}!") unless defined? XDo::Keyboard
        #Emit window manager keystroke for minimize all/show desktop
        XDo::Keyboard.key(get_window_manager_keystroke('show_desktop'))
      end
      
      #Minimizes the active window. There's no way to restore a specific minimized window.
      #Available after requireing "xdo/keyboard".
      #===Return value
      #Undefined.
      #===Raises
      #[NotImplementedError] You didn't require 'xdo/keyboard'.
      #[LoadError] Unable to get gconf value, check key set and gconftool-2 installed
      #===Example
      #  XDo::XWindow.minimize
      def minimize
        raise(NotImplementedError, "You have to require 'xdo/keyboard' before you can use #{__method__}!") unless defined? XDo::Keyboard
        #Emit window manager keystroke to minimize window
        XDo::Keyboard.key(get_window_manager_keystroke('minimize'))
      end
      
      #Maximize or normalize the active window if already maximized.
      #Available after requireing "xdo/keyboard".
      #===Return value
      #Undefined.
      #===Raises
      #[NotImplementedError] You didn't require 'xdo/keyboard'.
      #[LoadError] Unable to get gconf value, check key set and gconftool-2 installed
      #===Example
      #  XDo::XWindow.minimize
      #  XDo::XWindow.toggle_maximize
      def toggle_maximize
        raise(NotImplementedError, "You have to require 'xdo/keyboard' before you can use #{__method__}!") unless defined? XDo::Keyboard
        #Get window manager keystroke to maximize window
        XDo::Keyboard.key(get_window_manager_keystroke('maximize'))
      end

    #Returns keystroke string to control window manager (maximise, show desktop...)
    #Currently only supports Metacity, and hence reads gconf settings.
    def get_window_manager_keystroke(name)
      #Get Metacity keystroke from gconf. Need to check 2 directories for keys
      dirs = ['window_keybindings', 'global_keybindings']
      root = '/apps/metacity'
      keystroke = ''
      out = ''
      err = ''
      error_message = "Unable to determine #{name} keyboard shortcut from Metacity - check that gconftool-2 is installed\n"

      dirs.each do |d|
        key = "#{root}/#{d}/#{name}"
        return_code, out, err = XDo.execute("#{GCONFTOOL} -g #{key}")

        #If key doesn't exist, gconftool prints a message (including the requested key) saying so.
        if err.empty? or !out.empty?
          keystroke = out.to_s
          break
        end
      end

      #Bail if no keystroke found
      Kernel.raise(LoadError, error_message) if keystroke.empty?

      #+xdotool+ doesn't recognise '<Mod4>', use '<Super>' instead
      keystroke.gsub!('<Mod4>','<Super>')
      #Edit keystroke string to suit +xdotool+'s Keyboard.key command
      keystroke.gsub!('<','').gsub!('>','+')
      return keystroke
    end
    end
    
    ##
    #  :method: title=
    #call-seq:
    #  title = str
    #  name = str
    #
    #Changes a window's title.
    #===Parameters
    #[+str+] The new title.
    #===Return value
    #+str+.
    #===Raises
    #[XError] Error invoking +xdotool+.
    #===Example
    #  xwin.title = "Ruby is awesome!"
    
    ##
    # :method: icon_title=
    #call-seq:
    #  icon_title = str
    #  icon_name = str
    #
    #Changes the window's icon title, i.e. the string that is displayed in
    #the task bar or panel where all open windows show up.
    #===Parameters
    #[+str+] The string you want to set.
    #===Return value
    #+str+.
    #===Raises
    #[XError] Error invoking +xdotool+.
    #===Example
    #  xwin.icon_title = "This is minimized."
    
    ##
    #  :method: classname=
    #call-seq:
    #  classname = str
    #
    #Sets a window's classname.
    #===Parameters
    #[+str+] The window's new classname.
    #===Return value
    #+str+.
    #===Raises
    #[XError] Error invoking +xdotool+.
    #===Example
    #  xwin.classname = "MyNewClass"
    
    #Creates a new XWindow object from an internal ID.
    #===Parameters
    #[+id+] The internal ID to create the window from.
    #===Return value
    #The newly created XWindow object.
    #===Example
    #  id = XWindow.search(/edit/)[1]
    #  xwin = XWindow.new(id)
    #===Remarks
    #See also many class methods of the XWindow class which allow
    #you to forget about the internal ID of a window.
    def initialize(id)
      @id = id.to_i
    end
    
    #Human-readable output of form
    #  <XDo::XWindow: "title" (window_id)>
    def inspect
      %Q|<#{self.class}: "#{title}" (#{id})>|
    end
    
    #Set the size of a window.
    #===Parameters
    #[+width+] The new width, usually in pixels.
    #[+height+] The new height, usually in pixels.
    #[+use_hints+] (false) If true, window sizing hints are used if they're available. This is usually done when resizing terminal windows to a specific number of rows and columns.
    #[+sync+] (true) If true, this method blocks until the window has finished resizing. 
    #===Return value
    #Undefined.
    #===Raises
    #[XError] Error executing +xdotool+.
    #===Example
    #  #Resize a window to 400x300px
    #  xwin.resize(400, 300)
    #  #Resize a terminal window to 100 rows and 100 columns
    #  xtermwin.resize(100, 100, true)
    #===Remarks
    #This has no effect on maximized winwows.
    def resize(width, height, use_hints = false, sync = true)
      opts = []
      opts << "--usehints" if use_hints
      opts << "--sync" if sync
      return_code, out, err = XDo.execute("#{XDo::XDOTOOL} windowsize #{opts.join(" ")} #{@id} #{width} #{height}")
      Kernel.raise(XDo::XError, err) unless err.empty?
    end
    
    #Moves a window. +xdotool+ is not really exact with the coordinates,
    #special windows like Ubuntu's panels make it placing wrong. 
    #===Parameters
    #[+x+] The goal X coordinate.
    #[+y+] The goal Y coordinate.
    #[+sync+] (true) If true, this method blocks until the window has finished moving. 
    #===Return value
    #Undefined.
    #===Raises
    #[XError] Error executing +xdotool+.
    #===Example
    #  xwin.move(100, 100)
    #  p xwin.abs_position #=> [101, 101]
    def move(x, y, sync = true)
      opts = []
      opts << "--sync" if sync
      return_code, out, err = XDo.execute("#{XDo::XDOTOOL} windowmove #{opts.join(" ")} #{@id} #{x} #{y}")
      Kernel.raise(XDo::XError, err) unless err.empty?
    end
    
    #Set the input focus to the window (but don't bring it to the front).
    #===Parameters
    #[+sync+] (true) If true, this method blocks until the window got the input focus. 
    #===Return value
    #Undefined.
    #===Raises
    #[XError] Error invoking +xdotool+.
    #===Example
    #  xwin.focus
    #===Remarks
    #This method may not work on every window manager. You should use
    ##activate, which is supported by more window managers.
    def focus(sync = true)
      opts = []
      opts << "--sync" if sync
      return_code, out, err = XDo.execute("#{XDo::XDOTOOL} windowfocus #{opts.join(" ")} #{@id}")
      Kernel.raise(XDo::XError, err) unless err.empty?
    end
    
    #The window loses the input focus by setting it to an invalid window.
    #Parameters
    #[+sync+] (true) If true, this method blocks until the focus has been set to nothing. 
    #===Return value
    #Undefined.
    #===Example
    #  xwin.focus
    #  xwin.unfocus    
    def unfocus(sync = true)
      XDo::XWindow.unfocus(sync)
    end
    
    #Maps a window to the screen (makes it visible).
    #===Parameters
    #[+sync+] (true) If true, this method blocks until the window has been mapped. 
    #===Return value
    #Undefined.
    #===Raises
    #[XError] Error invoking +xdotool+.
    #===Example
    #  xwin.unmap #Windows are usually mapped
    #  xwin.map
    def map(sync = true)
      opts = []
      opts << "--sync" if sync
      return_code, out, err = XDo.execute("#{XDo::XDOTOOL} windowmap #{opts.join(" ")} #{@id}")
      Kernel.raise(XDo::XError, err) unless err.empty?
    end
    
    #Unmap a window from the screen (make it invisible).
    #===Parameters
    #[+sync+] (true) If true, this method blocks until the window has been unmapped. 
    #===Return value
    #Undefined.
    #===Raises
    #[XError] Error executing +xdotool+.
    #===Example
    #  xwin.unmap
    def unmap(sync = true)
      opts = []
      opts << "--sync" if sync
      return_code, out, err = XDo.execute("#{XDo::XDOTOOL} windowunmap #{opts.join(" ")} #{@id}")
      Kernel.raise(XDo::XError, err) unless err.empty?
    end
    
    #Bring a window to the front (but don't give it the input focus).
    #Not implemented in all window managers.
    #===Return value
    #Undefined.
    #===Raises
    #[XError] Error executing +xdotool+.
    #===Example
    #  xwin.raise
    def raise
      return_code, out, err = XDo.execute("#{XDo::XDOTOOL} windowraise #{@id}")
      Kernel.raise(XDo::XError, err) unless err.empty?
    end
    
    #Activate a window. That is, bring it to top and give it the input focus.
    #===Parameters
    #[+sync+] (true) If true, this method blocks until the window has been activated. 
    #===Return value
    #Undefined.
    #===Raises
    #[XError] Error executing +xdotool+.
    #===Example
    #  xwin.activate
    #===Remarks
    #This is the recommanded method to give a window the input focus, since
    #it works on more window managers than #focus and also works across
    #desktops.
    #
    #Part of the EWMH standard ACTIVE_WINDOW.
    def activate(sync = true)
      tried_focus = false
      begin
        opts = []
        opts << "--sync" if sync
        return_code, out, err = XDo.execute("#{XDo::XDOTOOL} windowactivate #{opts.join(" ")} #{@id}")
        Kernel.raise(XDo::XError, err) unless err.empty?
      rescue XDo::XError
        #If no window is active, xdotool's windowactivate fails, 
        #because it tries to determine which is the currently active window. 
        unless tried_focus
          tried_focus = true
          focus
          retry
        else
          raise
        end
      end
    end
    
    #Move a window to a desktop.
    #===Parameters
    #[+num+] The 0-based index of the desktop you want the window to move to.
    #===Return value
    #Undefined.
    #===Raises
    #[XError] Error executing +xdotool+.
    #===Example
    #  xwin.desktop = 3
    #===Remarks
    #Although Ubuntu systems seem to have several desktops, that isn't completely true. An usual Ubuntu system only
    #has a single working desktop, on which Ubuntu sets up an arbitrary number of other "desktop views" (usually 4).
    #That's kind of cheating, but I have not yet find out why it is like that. Maybe it's due to the nice cube rotating effect?
    #That's the reason, why the desktop-related methods don't work with Ubuntu.
    #
    #Part of the EWMH standard CURRENT_DESKTOP.
    def desktop=(num)
      return_code, out, err = XDo.execute("#{XDo::XDOTOOL} set_desktop_for_window #{@id} #{num}")
      Kernel.raise(XDo::XError, err) unless err.empty?
    end
    
    #Get the desktop the window is on.
    #===Return value
    #The 0-based index of the desktop this window resides on.
    #===Raises
    #[XError] Error executing +xdotool+.
    #===Example
    #  p xwin.desktop #=> 0
    #===Remarks
    #Although Ubuntu systems seem to have several desktops, that isn't completely true. An usual Ubuntu system only
    #has a single working desktop, on which Ubuntu sets up an arbitrary number of other "desktop views" (usually 4).
    #That's kind of cheating, but I have not yet find out why it is like that. Maybe it's due to the nice cube rotating effect?
    #That's the reason, why the desktop-related methods don't work with Ubuntu.
    #
    #Part of the EWMH standard CURRENT_DESKTOP.
    def desktop
      return_code, out, err = XDo.execute("#{XDo::XDOTOOL} get_desktop_for_window #{@id}")
      Kernel.raise(XDo::XError, err) unless err.empty?
      return out.to_i
    end
    
    #The title of the window or nil if it doesn't have a title.
    #===Return value
    #The window's title, encoded as UTF-8, or nil if the window doesn't have a title.
    #===Raises
    #[XError] Error executing +xwininfo+.
    #===Example
    #  p xwin.title #=> "xwindow.rb SciTE"
    def title
      if @id == XWindow.root_id #This is the root window
        return "(the root window)"
      elsif @id.zero?
        return "(NULL window)"
      else
        return_code, out, err = XDo.execute("#{XDo::XWININFO} -id #{@id}")
      end
      Kernel.raise(XDo::XError, err) unless err.empty?
      title = out.strip.lines.to_a[0].match(/"(.*)"/)[1] rescue Kernel.raise(XDo::XError, "No window with ID #{@id} found!")
      return title #Kann auch nil sein, dann ist das Fenster namenlos.
    end
    
    #The absolute position of the window on the screen.
    #===Return value
    #A two-element array of form <tt>[x, y]</tt>.
    #===Raises
    #[XError] Error executing +xwininfo+.
    #===Example
    #  p xwin.abs_position #=> [0, 51]
    def abs_position
      return_code, out, err = XDo.execute("#{XDo::XWININFO} -id #{@id}")
      Kernel.raise(XDo::XError, err) unless err.empty?
      out = out.strip.lines.to_a
      x = out[2].match(/:\s+(\d+)/)[1]
      y = out[3].match(/:\s+(\d+)/)[1]
      [x.to_i, y.to_i]
    end
    alias position abs_position
    
    #The position of the window relative to it's parent window.
    #===Return value
    #A two-element array of form <tt>[x, y]</tt>.
    #===Raises
    #[XError] Error executing +xdotool+.
    #===Example
    #  p xwin.rel_position => [0, 51]
    def rel_position
      return_code, out, err = XDo.execute("#{XDo::XWININFO} -id #{@id}")
      Kernel.raise(XDo::XError, err) unless err.empty?
      out = out.strip.lines.to_a
      x = out[4].match(/:\s+(\d+)/)[1]
      y = out[5].match(/:\s+(\d+)/)[1]
      [x.to_i, y.to_i]
    end
    
    #The size of the window.
    #===Return value
    #A two-element array of form <tt>[width, height]</tt>.
    #===Raises
    #[XError] Error executing +xwininfo+.
    #===Example
    #  p xwin.size #=> [1280, 948]
    def size
      return_code, out, err = XDo.execute("#{XDo::XWININFO} -id #{@id}")
      out = out.strip.lines.to_a
      Kernel.raise(XDo::XError, err) unless err.empty?
      width = out[6].match(/:\s+(\d+)/)[1]
      height = out[7].match(/:\s+(\d+)/)[1]
      [width.to_i, height.to_i]
    end
    
    #true if the window is mapped to the screen.
    #===Return value
    #nil if the window is not mapped, an integer value otherwise.
    #===Raises
    #[XError] Error executing +xwininfo+.
    #===Example
    #  p xwin.visible? #=> 470
    #  xwin.unmap
    #  p xwin.visible? #=> nil
    def visible?
      return_code, out, err = XDo.execute("#{XDo::XWININFO} -id #{@id}")
      out = out.strip
      Kernel.raise(XDo::XError, err) unless err.empty?
      return out =~ /IsViewable/
    end
    
    #Returns true if the window exists.
    #===Return value
    #true or false.
    #===Example
    #  p xwin.exists? #=> true
    def exists?
      XDo::XWindow.id_exists?(@id)
    end
    
    #Closes a window by activating it, and sending it the close keystroke (obtained
    #from the window manager's settings, usually [ALT] + [F4]).
    #===Return value
    #nil.
    #===Raises
    #[NotImplementedError] You didn't require "xdo/keyboard".
    #[LoadError] Unable to get gconf value, check key set and gconftool-2 installed
    #===Example
    #  xwin.close
    #===Remarks
    #A program could ask to save data.
    #
    #Use #kill! to kill the process running the window.
    #
    #Available after requireing "xdo/keyboard".
    def close
      Kernel.raise(NotImplementedError, "You have to require 'xdo/keyboard' before you can use #{__method__}!") unless defined? XDo::Keyboard
      activate
      XDo::Keyboard.char(self.class.get_window_manager_keystroke('close'))
      sleep 0.5
      nil
    end
    
    #More aggressive variant of #close. Think of +close!+ as
    #the middle between #close and #kill!. It first tries
    #to close the window by calling #close and if that
    #does not succeed (within +timeout+ seconds), it will call #kill!.
    #===Paramters
    #[+timeout+] (2) The time to wait before using #kill!, in seconds.
    #===Return value
    #Undefined.
    #===Raises
    #[XError] Error executing +xkill+.
    #===Example
    #  xwin.close!
    #===Remarks
    #Available after requireing "xdo/keyboard".
    def close!(timeout = 2)
      Kernel.raise(NotImplementedError, "You have to require 'xdo/keyboard' before you can use #{__method__}!") unless defined? XDo::Keyboard
      #Try to close normally
      close
      #Check if it's deleted
      if exists?
        #If not, wait some seconds and then check again
        sleep timeout
        if exists?
          #If it's not deleted after some time, force it to close.
          kill!
        end
      end
    end
    
    #Kills the process that runs a window. The window will be
    #terminated immediatly, if that isn't what you want, have
    #a look at #close.
    #===Return value
    #nil.
    #===Raises
    #[XError] Error executing +xkill+.
    #===Example
    #  xwin.kill!
    def kill!
      return_code, out, err = XDo.execute("#{XDo::XKILL} -id #{@id}")
      Kernel.raise(XDo::XError, err) unless err.empty?
      nil
    end
    
    #Returns the window's internal ID.
    #===Return value
    #An integer describing the window's internal ID.
    #===Example
    #  p xwin.to_i #=> 29361095
    def to_i
      @id
    end
    
    #Returns a window's title.
    #===Return value
    #The window's title.
    #===Example
    #  p xwin.to_s #=> "xwindow.rb * SciTE"
    def to_s
      title
    end
    
    #true if the internal ID is zero.
    #===Return value
    #true or false.
    #===Example
    #  p xwin.zero? #=> false
    def zero?
      @id.zero?
    end
    
    #true if the internal ID is not zero.
    #===Return value
    #nil or the internal ID.
    #===Example
    #  p xwin.nonzero? #=> 29361095
    def nonzero?
      @id.nonzero?
    end
    
    [:"name=", :"icon_name=", :"classname="].each do |sym|
      define_method(sym) do |str|
        set_window(sym.to_s[0..-2].gsub("_", "-"), str.encode("UTF-8"))
        str
      end
    end
    alias title= name=
    alias icon_title= icon_name=
    
    private
    
    #Calls +xdotool+'s set_window command with the given options.
    def set_window(option, value)
      return_code, out, err = XDo.execute("#{XDOTOOL} set_window --#{option} '#{value}' #{@id}")
      Kernel.raise(XDo::XError, err) unless err.empty?
    end
    
  end
  
end
