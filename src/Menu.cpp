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

    // Always start with the main menu
    if (!_menu)
        _menu = &_menuTable[0];

    // Process the command
    if (command)
    {
        // Walk the menu table
        menuEntry = _menu->firstEntry;
        menuEnd = &menuEntry[_menu->menuEntryCount];
        while (menuEntry < menuEnd)
        {
            // Determine if the command has parameters
            cmd = menuEntry->command;
            if (menuEntry->align)
            {
                length = strlen(cmd);
                found = (r4aStrincmp(command, cmd, length) == 0)
                    && (command[length] <= ' ');
            }
            else
                found = (r4aStricmp(command, cmd) == 0);

            // Determine if the command was found
            if (found)
            {
                // Process the command
                if (menuEntry->menuRoutine)
                    menuEntry->menuRoutine(menuEntry, command, display);
                else
                {
                    // Get the next menu index
                    uint32_t index = (int)menuEntry->menuParameter;

                    // Validate the next menu index
                    if (index <= _menuTableEntries)
                    {
                        // Select the next menu
                        if (index)
                            _menu = &_menuTable[index - 1];

                        // Exit the menu system
                        else
                            _menu = nullptr;
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

        if (_menu->preMenu)
        {
            // Separate the preMenu from previous output
            if (_blankLineBeforePreMenu)
                display->println();

            // Display the data before the menu
            _menu->preMenu(display);
        }

        // Separate the preMenu display from the menu header
        if (_blankLineBeforeMenuHeader)
            display->println();

        // Display the menu header
        display->printf("%s\r\n", _menu->menuName);
        for (int length = strlen(_menu->menuName); length > 0; length--)
            display->print("-");
        display->println();

        // Separate the menu header from the commands
        if (_blankLineAfterMenuHeader)
            display->println();

        // Determine the maximum command length
        maxLength = 0;
        if (_alignCommands)
        {
            menuEntry = _menu->firstEntry;
            menuEnd = &menuEntry[_menu->menuEntryCount];
            while (menuEntry < menuEnd)
            {
                length = strlen(menuEntry->command) + menuEntry->align;
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
            // Align the menu items
            if (_alignCommands)
            {
                align = String("");
                length = strlen(menuEntry->command) + menuEntry->align;
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
    return (_menu == nullptr);
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
