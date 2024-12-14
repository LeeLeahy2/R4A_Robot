/**********************************************************************
  Menu.ino

  Robots-For-All (R4A)
  Menu support
**********************************************************************/

#include "R4A_Robot.h"

//*********************************************************************
// Process the menu command or display the menu
// Returns true when exiting the menu system
bool R4A_MENU::process(const char * command,
                       Print * display)
{
    String align("");
    int alignSpaces;
    const char * cmd;
    bool found;
    int length;
    int maxLength;
    const R4A_MENU_ENTRY * menuEnd;
    const R4A_MENU_ENTRY * menuEntry;
    const char * spaces = "                                                  ";
    int spaceCount;

    if (_debug)
    {
        Serial.printf("command: %p %s%s%s\r\n",
                      command,
                      command ? "(" : "",
                      command ? command : "",
                      command ? ")" : "");
        Serial.printf("display: %p\r\n", display);
        Serial.printf("_menu: %p\r\n", _menu);
    }

    // Always start with the main menu
    if (!_menu)
    {
        _menu = &_menuTable[0];
        if (_debug)
            Serial.printf("_menu: %p\r\n", _menu);
    }

    // Process the command
    if (command)
    {
        // Walk the menu table
        menuEntry = _menu->firstEntry;
        menuEnd = &menuEntry[_menu->menuEntryCount];
        while (menuEntry < menuEnd)
        {
            if (_debug)
                Serial.printf("menuEntry: %p\r\n", menuEntry);

            // Determine if the command has parameters
            cmd = menuEntry->command;
            if (_debug)
                Serial.printf("menuEntry->command: %p %s%s%s\r\n",
                              cmd,
                              cmd ? "(" : "",
                              cmd ? cmd : "",
                              cmd ? ")" : "");
            if (menuEntry->align)
            {
                length = strlen(cmd);
                found = (r4aStrincmp(command, cmd, length) == 0);
            }
            else
                found = (r4aStricmp(command, cmd) == 0);
            if (_debug)
                Serial.printf("found: %d\r\n", found);

            // Determine if the command was found
            if (found)
            {
                if (_debug)
                    Serial.printf("menuEntry->menuRoutine: %p\r\n", menuEntry->menuRoutine);

                // Process the command
                if (menuEntry->menuRoutine)
                {
                    if (_debug)
                    {
                        Serial.printf("menuEntry: %p\r\n", menuEntry);
                        Serial.printf("command: %p %s%s%s\r\n",
                                      command,
                                      command ? "(" : "",
                                      command ? command : "",
                                      command ? ")" : "");
                        Serial.printf("display: %p\r\n", display);
                    }
                    menuEntry->menuRoutine(menuEntry, command, display);
                }
                else
                {
                    // Get the next menu index
                    uint32_t index = (int)menuEntry->menuParameter;
                    if (_debug)
                    {
                        Serial.printf("index: %ld\r\n", index);
                        Serial.printf("_menuTableEntries: %d\r\n", _menuTableEntries);
                    }

                    // Validate the next menu index
                    if (index <= _menuTableEntries)
                    {
                        // Select the next menu
                        if (index)
                            _menu = &_menuTable[index - 1];

                        // Exit the menu system
                        else
                            _menu = nullptr;
                        if (_debug)
                            Serial.printf("_menu: %p\r\n", _menu);
                    }
                    else
                        r4aReportFatalError("Invalid menu index!");
                }
                break;
            }
            menuEntry++;
        }

        // Display the error message
        if ((menuEntry >= menuEnd) && strlen(command))
            display->println("Error: Invalid command entered");
    }
    else
    {
        // Start at the beginning of the line
        display->print('\r');

        if (_debug)
            Serial.printf("_menu->preMenu: %p\r\n", _menu->preMenu);
        if (_menu->preMenu)
        {
            // Separate the preMenu from previous output
            if (_debug)
                Serial.printf("_blankLineBeforePreMenu: %d\r\n", _blankLineBeforePreMenu);
            if (_blankLineBeforePreMenu)
                display->println();

            // Display the data before the menu
            _menu->preMenu(display);
        }

        // Separate the preMenu display from the menu header
        if (_debug)
            Serial.printf("_blankLineBeforeMenuHeader: %d\r\n", _blankLineBeforeMenuHeader);
        if (_blankLineBeforeMenuHeader)
            display->println();

        // Display the menu header
        if (_debug)
            Serial.printf("_menu->menuName: %p %s%s%s\r\n",
                          _menu->menuName,
                          _menu->menuName ? "(" : "",
                          _menu->menuName ? _menu->menuName : "",
                          _menu->menuName ? ")" : "");
        display->printf("%s\r\n", _menu->menuName);
        for (int length = strlen(_menu->menuName); length > 0; length--)
            display->print("-");
        display->println();

        // Separate the menu header from the commands
        if (_debug)
            Serial.printf("_blankLineAfterMenuHeader: %d\r\n", _blankLineAfterMenuHeader);
        if (_blankLineAfterMenuHeader)
            display->println();

        // Determine the maximum command length
        if (_debug)
        {
            Serial.printf("_alignCommands: %d\r\n", _alignCommands);
            Serial.printf("_menu->menuEntryCount: %ld\r\n", _menu->menuEntryCount);
            Serial.printf("_menu->firstEntry: %p\r\n", _menu->firstEntry);
        }
        maxLength = 0;
        if (_alignCommands)
        {
            menuEntry = _menu->firstEntry;
            menuEnd = &menuEntry[_menu->menuEntryCount];
            while (menuEntry < menuEnd)
            {
                length = strlen(menuEntry->command) + menuEntry->align + 1;
                if (maxLength < length)
                    maxLength = length;
                menuEntry++;
            }
        }

        // Display the menu items
        menuEntry = _menu->firstEntry;
        menuEnd = &menuEntry[_menu->menuEntryCount];
        while (menuEntry < menuEnd)
        {
            if (_debug)
            {
                Serial.printf("menuEntry: %p\r\n", menuEntry);
                Serial.printf("menuEntry->command: %p %s%s%s\r\n",
                              menuEntry->command,
                              menuEntry->command ? "(" : "",
                              menuEntry->command ? menuEntry->command : "",
                              menuEntry->command ? ")" : "");
                Serial.printf("menuEntry->align: %d\r\n", menuEntry->align);
                Serial.printf("menuEntry->helpRoutine: %p\r\n", menuEntry->helpRoutine);
            }

            // Align the menu items
            if (_alignCommands)
            {
                align = String("");
                length = strlen(menuEntry->command) + menuEntry->align
                       + (menuEntry->align ? 1 : 0);
                spaceCount = strlen(spaces);
                alignSpaces = maxLength - length;
                while (alignSpaces > 0)
                {
                    length = alignSpaces;
                    if (length > spaceCount)
                        length = spaceCount;
                    align += &spaces[spaceCount - length];
                    alignSpaces -= length;
                }
            }

            // Display the menu item
            if (menuEntry->helpRoutine)
                menuEntry->helpRoutine(menuEntry, align.c_str(), display);
            else
                display->printf("%s: %s%s\r\n", menuEntry->command,
                                align.c_str(), menuEntry->helpText);
            menuEntry++;
        }

        // Separate the menu from any following text
        if (_blankLineAfterMenu)
            display->println();
    }

    // Determine if exiting the menu system
    if (_debug)
        Serial.printf("_menu: %p\r\n", _menu);
    return (_menu == nullptr);
}

//*********************************************************************
// Enable or disable debugging
void R4A_MENU::debug(bool enable)
{
    _debug = enable;
}

//*********************************************************************
// Display the menu object contents
void R4A_MENU::display(Print * display)
{
    display->printf("Menu @ %p\r\n", this);
    display->printf("    _menu: %p\r\n", _menu);
    display->printf("    _menuTable: %p\r\n", _menuTable);
    display->printf("    _menuTableEntries: %d\r\n", _menuTableEntries);
    display->printf("    _blankLineBeforePreMenu: %d\r\n", _blankLineBeforePreMenu);
    display->printf("    _blankLineBeforeMenuHeader: %d\r\n", _blankLineBeforeMenuHeader);
    display->printf("    _blankLineAfterMenuHeader: %d\r\n", _blankLineAfterMenuHeader);
    display->printf("    _alignCommands: %d\r\n", _alignCommands);
    display->printf("    _blankLineAfterMenu: %d\r\n", _blankLineAfterMenu);
}

//*********************************************************************
// Display the boolean as enabled or disabled
void r4aMenuBoolHelp(const R4A_MENU_ENTRY * menuEntry, const char * align, Print * display)
{
    bool * value = (bool *)menuEntry->menuParameter;
    display->printf("%s: %s%s %s\r\n", menuEntry->command, align,
                    menuEntry->helpText, *value ? "Enabled" : "Disabled");
}

//*********************************************************************
// Toggle boolean value
void r4aMenuBoolToggle(const R4A_MENU_ENTRY * menuEntry, const char * command, Print * display)
{
    bool * value = (bool *)menuEntry->menuParameter;
    *value ^= 1;
}

//*********************************************************************
// Get the string of parameters
String r4aMenuGetParameters(const struct _R4A_MENU_ENTRY * menuEntry,
                            const char * command)
{
    // Get the parameter name
    String line = String(&command[strlen(menuEntry->command)]);

    // Strip white space from the beginning and the end of the name
    line.trim();
    return line;
}

//*********************************************************************
// Display the menu item with a suffix and help text.  The suffix is
// specified as the menu parameter.
// Inputs:
//   menuEntry: Address of the object describing the menu entry
//   align: Zero terminated string of spaces for alignment
//   display: Device used for output
void r4aMenuHelpSuffix(const struct _R4A_MENU_ENTRY * menuEntry,
                       const char * align,
                       Print * display)
{
    display->printf("%s %s: %s%s\r\n",
                    menuEntry->command,
                    (const char *)menuEntry->menuParameter,
                    align,
                    menuEntry->helpText);
}
