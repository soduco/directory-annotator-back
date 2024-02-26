#JSON format description

## Document Model

The content is stored as a list of json element wich is differencied by the value contained in the key "type".

Each element as his own form, required attribute and optional attribute which will allow the user to use old format content stored in the user storage or in the server if possible.

## Element Model

```
element PAGE {
    required int32 id;
    required string type = "PAGE";    
    repeated int32 box = [x, y, width, height];
}

element SECTION_LEVEL_1 {
    required int32 id;
    required int32 parent;
    required string type = "SECTION_LEVEL_1";
    repeated int32 box = [x, y, width, height];
}

element COLUMN_LEVEL_1 {
    required int32 id;
    required int32 parent;
    required string type = "COLUMN_LEVEL_1";
    repeated int32 box = [x, y, width, height];
}

element TITLE_LEVEL_1 {
    required int32 id;
    required int32 parent;
    required string type = "TITLE_LEVEL_1";
    repeated int32 box = [x, y, width, height];
    required string text;
    optional string origin = "human" or "computer" [DEFAULT = "computer"];
    optional bool checked [DEFAULT = False]; 
}

element SECTION_LEVEL_2 {
    required int32 id;
    required int32 parent;
    required string type = "SECTION_LEVEL_2";
    repeated int32 box = [x, y, width, height];
}

element COLUMN_LEVEL_2 {
    required int32 id;
    required int32 parent;
    required string type = "COLUMN_LEVEL_2";
    repeated int32 box = [x, y, width, height];
}

element TITLE_LEVEL_2 {
    required int32 id;
    required int32 parent;
    required string type = "TITLE_LEVEL_2";
    repeated int32 box = [x, y, width, height];
    required string text;
    optional string origin = "human" or "computer" [DEFAULT = "computer"];
    optional bool checked [DEFAULT = False]; 
}

element ENTRY {
    required int32 id;
    required int32 parent;
    required string type = "ENTRY";
    repeated int32 box = [x, y, width, height];
    required string raw;
    required string name;
    required string street;
    required int32 street_number;
    required int32 grade;
    required bool perfect_match;
    optional string origin = "human" or "computer" [DEFAULT = "computer"];
    optional bool checked [DEFAULT = False]; 
}

element LINE {
    required int32 id;
    required int32 parent;
    required string type = "LINE";
    repeated int32 box = [x, y, width, height];
    required string text;
    required bool indented;
    required bool EOL;
    optional string origin = "human" or "computer" [DEFAULT = "computer"];
    optional bool checked [DEFAULT = False]; 
}
```
