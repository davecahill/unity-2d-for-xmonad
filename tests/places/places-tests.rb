#!/usr/bin/env ruby1.8
=begin
/*
 * This file is part of unity-2d
 *
 * Copyright 2012 Canonical Ltd.
 *
 * Authors:
 * - Albert Astals Cid <albert.astals@canonical.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
=end

require '../run-tests.rb' unless $INIT_COMPLETED
require 'xdo/xwindow'
require 'xdo/keyboard'
require 'xdo/mouse'
require 'tmpwindow'

############################# Test Suite #############################
context "Dash Tests" do
    WIDTH = 65
  # Run once at the beginning of this test suite
  startup do
    $SUT.execute_shell_command 'killall unity-2d-places'
    $SUT.execute_shell_command 'killall unity-2d-places'

    $SUT.execute_shell_command 'killall unity-2d-launcher'
    $SUT.execute_shell_command 'killall unity-2d-launcher'

    # Minimize all windows
    XDo::XWindow.toggle_minimize_all
  end

  # Run once at the end of this test suite
  shutdown do
  end

  # Run before each test case begins
  setup do
    # Execute the application 
    @app_launcher = $SUT.run( :name => UNITY_2D_LAUNCHER, 
                              :arguments => "-testability", 
                              :sleeptime => 2 )

    @app_places = $SUT.run( :name => UNITY_2D_PLACES, 
                              :arguments => "-testability", 
                              :sleeptime => 2 )

  end

  # Run after each test case completes
  teardown do
    TmpWindow.close_all_windows
    #Need to kill Launcher as it does not shutdown when politely asked
    $SUT.execute_shell_command 'pkill -nf unity-2d-launcher'
    $SUT.execute_shell_command 'pkill -nf unity-2d-places'
  end

  #####################################################################################
  # Test casess

  # Test case objectives:
  #   * Check that Alt+F2 shows dash
  # Pre-conditions
  #   * Desktop with no running applications
  # Test steps
  #   * Verify dash is not showing
  #   * Press Alt+F2
  #   * Verify dash is showing
  # Post-conditions
  #   * None
  # References
  #   * None
  test "Alt+F2 shows the Dash" do
    verify_not(2, 'There should not be a Dash declarative view on startup') {
      @app_places.DashDeclarativeView()
    }
    XDo::Keyboard.alt_F2 #Must use uppercase F to indicate function keys
    verify(TIMEOUT, 'There should be a Dash declarative view after pressing Alt+F2') {
      @app_places.DashDeclarativeView()
    }
  end

  # Test case objectives:
  #   * Check that pressing the bfb shows the dash
  # Pre-conditions
  #   * Desktop with no running applications
  # Test steps
  #   * Verify dash is not showing
  #   * Click on the bfb
  #   * Verify dash is showing
  # Post-conditions
  #   * None
  # References
  #   * None
  test "Pressing the bfb shows the Dash" do
    verify_not(2, 'There should not be a Dash declarative view on startup') {
      @app_places.DashDeclarativeView()
    }
    bfb = @app_launcher.LauncherList( :name => 'main' ).LauncherList( :isBfb => true );
    XDo::Mouse.move(0, 200, 0, true)
    verify_equal( 0, TIMEOUT, 'Launcher hiding when mouse at left edge of screen' ) {
      @app_launcher.Unity2dPanel()['x_absolute'].to_i
    }
    bfb.move_mouse()
    bfb.tap()
    verify(TIMEOUT, 'There should be a Dash declarative view after activating the bfb') {
        @app_places.DashDeclarativeView()
    }
  end

  # Test case objectives:
  #   * Check Super and Alt+F1 interaction
  # Pre-conditions
  #   * Desktop with no running applications
  # Test steps
  #   * Verify dash is not showing
  #   * Press Super
  #   * Verify dash is showing
  #   * Press Alt+F1
  #   * Verify dash is not showing
  #   * Verify launcher does not hide
  # Post-conditions
  #   * None
  # References
  #   * None
  test "Super and Alt+F1 interaction" do
    xid = TmpWindow.open_window_at(10,100)
    verify_not(2, 'There should not be a Dash declarative view on startup') {
      @app_places.DashDeclarativeView()
    }
    XDo::Keyboard.super
    verify(TIMEOUT, 'There should be a Dash declarative view after pressing Super') {
      @app_places.DashDeclarativeView()
    }
    XDo::Keyboard.alt_F1 #Must use uppercase F to indicate function keys
    verify_not(2, 'There should not be a Dash declarative view after pressing Alt+F1') {
      @app_places.DashDeclarativeView()
    }
    verify_not(0, 'Launcher should not hide after pressing Alt+F1') {
      verify_equal( -WIDTH, 2 ) {
        @app_launcher.Unity2dPanel()['x_absolute'].to_i
      }
    }
    xid.close!
  end

  # Test case objectives:
  #   * Check Super, Super and Alt+F1 interaction
  # Pre-conditions
  #   * Desktop with no running applications
  # Test steps
  #   * Verify dash is not showing
  #   * Verify terminal has focus
  #   * Press Super
  #   * Verify dash is showing
  #   * Press Super
  #   * Verify dash is not showing
  #   * Verify terminal has focus
  #   * Verify launcher is hidden
  #   * Press Alt+F1
  #   * Verify dash is not showing
  #   * Verify launcher shows
  # Post-conditions
  #   * None
  # References
  #   * None
  test "Super, Super and Alt+F1 interaction" do
    xid = TmpWindow.open_window_at(10,100)
    verify_equal( xid.id, TIMEOUT, 'terminal should have focus after starting it' ) {
      XDo::XWindow.active_window
    }
    verify_not(2, 'There should not be a Dash declarative view on startup') {
      @app_places.DashDeclarativeView()
    }
    XDo::Keyboard.super
    verify(TIMEOUT, 'There should be a Dash declarative view after pressing Super') {
      @app_places.DashDeclarativeView()
    }
    XDo::Keyboard.super
    verify_not(2, 'There should not be a Dash declarative view after pressing Super again') {
      @app_places.DashDeclarativeView()
    }
    verify_equal( xid.id, TIMEOUT, 'terminal should have focus after toggling the dash' ) {
      XDo::XWindow.active_window
    }
    verify_equal( -WIDTH, TIMEOUT, 'Launcher should be hiding after toggling the dash' ) {
      @app_launcher.Unity2dPanel()['x_absolute'].to_i
    }
    XDo::Keyboard.alt_F1 #Must use uppercase F to indicate function keys
    verify_not(2, 'There should not be a Dash declarative view after pressing Alt+F1') {
      @app_places.DashDeclarativeView()
    }
    verify_equal( 0, TIMEOUT, 'Launcher should be showing after pressing Alt+F1' ) {
      @app_launcher.Unity2dPanel()['x_absolute'].to_i
    }
    xid.close!
  end
end
