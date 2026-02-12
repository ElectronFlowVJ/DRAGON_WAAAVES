# Conditionally add ofxSpout on Windows only
ifeq ($(OS),Windows_NT)
PROJECT_ADDONS += ofxSpout
endif
