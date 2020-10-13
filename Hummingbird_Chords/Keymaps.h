byte aKeyMap[] =  { KEY_TAB,'d','r','w','b',  KEY_LEFT_CTRL,'f','u','p', KEY_BACKSPACE,
                        'a','s','h','t','g',            'y','n','e','o','i',
                            'm','c','v',                    'k','l','.',
                              ',',' ',                        ' ',KEY_RETURN };

byte aAltMap[] =  { KEY_ESC,     KEY_HOME,    KEY_UP_ARROW,      KEY_END,      KEY_PAGE_UP,   KEY_LEFT_ALT,'7','8','9', KEY_DELETE,
                 KEY_INSERT, KEY_LEFT_ARROW, KEY_DOWN_ARROW, KEY_RIGHT_ARROW, KEY_PAGE_DOWN,           '.','4','5','6','0',
                            'm','c','v',                                                                   '1','2','3',
                              '.','.',                                                                       ' ',KEY_RETURN };

byte aKeyMap2[] = { 'Q','W','E','R','T','Y','U','I','O','p',
                    'A','S','D','F','G','H','J','K','L','m',
                    'Z','X','C','V','B','N','<','>','?','?',' ',KEY_RETURN };

byte a2Chords0[] = { '.','.','.','.', 1, '-','q','[',']', 1,
                     '.','.','\\','`', 1, '=','/',';',KEY_APSOSTROPHE, 1,
                         'z','x',     1,     'j',',' ,1,1,1,0,0,0,0};

byte a2Chords1[] = { '.','.','.','.',  1, KEY_F7,KEY_F8,KEY_F9,KEY_F10, 1,
                     '.','.','.','.',  1, KEY_F3,KEY_F4,KEY_F5,KEY_F6,  1,
                      KEY_F11,KEY_F12, 1,        KEY_F1,KEY_F2 ,1,1,1,0,0,0,0};
                         
struct tKeyMap{
     byte     Pressed;
     byte     Mods;
     uint32_t Map;
     byte     Key;
};

byte RowPins[] = {0,1,2,3,4,5};
byte ColPins[] = {6,7,8,9,10};
byte Decode[]  = {0,1,10,11,20,21,2,3,12,13,22,26,4,5,14,15,27,23,6,7,16,17,24,25,8,9,18,19,29,28,30,31};

const int Rows = sizeof(RowPins)/sizeof(RowPins[0]);
const int Cols = sizeof(ColPins)/sizeof(ColPins[0]);

tKeyMap KeyMap, oKeyMap;
