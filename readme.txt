Table of Contents                                                             80.
==================
   1. Introduction
   2. Requirements
   3. Installation
      a) Installation from a .tar.gz
      b) Installation from github as a submodule
      c) Installation of the JSONcpp library from a .tar.gz
   4. Configuring Anope to use this module
   5. Configuring Apache to hide the secret services IP
   6. Configuring Your GitHub project to send the data to anope


1. Introduction
================
   This anope module listens for JSON commit data from github and uses an
   assigned BotServ bot to relay it into a channel.


2. Requirements
===============
   - a working Anope 1.9.8 or newer
   - the module m_httpd loaded and configured (its part of anopes core)
   - the jsoncpp library. its included with this module, but you can find the
     latest version always at http://jsoncpp.sourceforge.net/

3. Installation
===============
  a) Installation from a .tar.gz
     - put the anope-github.tar.gz file in anope-1.9/modules/extra
     - type 'tar -xzf anope-github.tar.gz', you should have a directory
       anope-1.9/modules/extra/anope-github/ now
     - go to anope-1.9/build/  and type "cmake ."
       cmake will detect the new files now
     - type 'make install'
  b) Installation from from github as a submodule
     - go to your anope-1.9-git/ directory and type
       "git submodule add git://github.com/Adam-/anope-github.git modules/extra/github"
       this creates a new directory anope-1.9-git/modules/extra/github
     - you can update this module by using the "git submodule update" command
     - go to anope-1.9/build/  and type "cmake ."
       cmake will detect the new files now
     - type 'make install'
   c) Installation of the JSONcpp library from a .tar.gz
       TODO: ask Adam

4. Configuring Anope to use this module
=======================================

   Edit your conf/modules.conf and add following lines.

      module { name = "m_github" }
      github { channel = "#anope"; repos = "anope windows-libs" }
      github { channel = "#denora"; repos = "denora anope" }

   Make sure you add this AFTER the m_httpd configuration.
   m_github always uses "httpd/main"

5. Configuring Apache to hide the secret services IP
====================================================
   Apache needs the mod_proxy module loaded.
   Add following lines to your apache virtualhost configuration. (root required)

      ProxyPass /api/github/ http://192.168.0.1:1234/github
      ProxyPassReverse /api/github/ http://192.168.0.1:1234/github

   (replace the IP/PORT with the IP of the services and the PORT configured in m_httpd)
   This forwards all requests to www.yoursite.net/api/github/ to anope.

6. Configuring Your GitHub project to send the data to anope
=============================================================
   In the admin panel of your repository, go to "Service Hooks" and enable "WebHook URLs".
   Put the path to www.yoursite.net/api/github/ (as configured in step 5) in and don't
   forget to press "Update Settings".
   Commit to your project and enjoy.

