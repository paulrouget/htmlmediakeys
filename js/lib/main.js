const keys = ["unknownerror",
              "mute",
              "play",
              "pause",
              "stop",
              "previous",
              "next"];

const URL = require("url");
const data = require("self").data;
const components = require("chrome").components;;
const ctypes = components.utils.import("resource://gre/modules/ctypes.jsm").ctypes;
const windows = require("windows");

//
// Send "media" events to all the web pages
// We use jsctypes and a small C library
//

var cbType = ctypes.FunctionType(ctypes.default_abi, ctypes.void_t, [ctypes.int32_t]);
var lib = ctypes.open(URL.toFilename(data.url("mediakeys.so")));
var init = lib.declare("init", ctypes.default_abi, ctypes.void_t, cbType.ptr);

function callback(key) {
    for each (var window in windows.browserWindows) {
        for each (var tab in window.tabs) {
            var name = keys[key];
            var evt = tab.contentDocument.createEvent("MessageEvent");
            evt.initMessageEvent("media", true, true, name, tab.contentDocument.location.href, "", tab.contentWindow);
            tab.contentDocument.dispatchEvent(evt);
        }
    }
}

var ptr = cbType.ptr(callback);
init(ptr);


//
// listen to the "media" events from the web pages
//

let tracker = {
  init: function() {
    let windowTracker = new (require("window-utils").WindowTracker)(this);
  },

  onTrack: function(window) {
      var document = window.document;
      var style = document.createElementNS("http://www.w3.org/1999/xhtml", "style");
      style.appendChild(document.createTextNode('.sound .tab-throbber {-moz-box-ordinal-group: 2!important; display: inline!important;}' +
                                                '.sound .tab-throbber {list-style-image: url(' + data.url('sound.png') + ')}' +
                                                '.sound .tab-icon-image {-moz-box-ordinal-group: 1!important;}' +
                                                '.sound .tab-close-button{-moz-box-ordinal-group: 4!important;}' +
                                                '.sound .tab-label {-moz-box-ordinal-group: 3!important;}'));
      style.setAttribute("type", "text/css"),
      document.documentElement.appendChild(style);
  },
  onUntrack: function(window) {}
};
tracker.init();

function listenToMediaEvents(tab) {
    var document = tab.contentDocument;
    document.addEventListener("media", function(e) {
        if (e.data == "sound") {
            tab.innerTab.classList.add("sound");
        }
        if (e.data == "nosound") {
            tab.innerTab.classList.remove("sound");
        }
    }, true);
}

var tabs = require("tabs");
tabs.on('ready', function onOpen(tab) {
        listenToMediaEvents(tab);
});
for each (var window in windows.browserWindows) {
    for each (var tab in window.tabs) {
        listenToMediaEvents(tab);
    }
}

//
// Page Mod for GrooveShark
//

var pageMod = require("page-mod");
pageMod.PageMod({
    include: "*.grooveshark.com",
    contentScriptWhen: 'ready',
    contentScript: '' +
'window.console.log = function(m) {' +
'    if (m === "PLAY_STATUS_PAUSED") {' +
'        var evt = document.createEvent("MessageEvent");' +
'        evt.initMessageEvent("media", true, true, "nosound", document.location.href, "", window);' +
'        document.dispatchEvent(evt);' +
'' +
'    }' +
'    if (m === "PLAY_STATUS_PLAYING") {' +
'        var evt = document.createEvent("MessageEvent");' +
'        evt.initMessageEvent("media", true, true, "sound", document.location.href, "", window);' +
'        document.dispatchEvent(evt);' +
'    }' +
'};' +
'window.addEventListener("media", function(e) {' +
'    if (e.data == "next") {' +
'        document.getElementById("player_next").click();' +
'    };' +
'    if (e.data == "play" || e.data == "stop") {' +
'        document.getElementById("player_play_pause").click();' +
'    };' +
'    if (e.data == "previous") {' +
'        document.getElementById("player_previous").click();' +
'    };' +
'}, true)'
});
