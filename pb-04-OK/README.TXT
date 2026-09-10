F -> forward
L -> turn left
R -> turn right

C -> cannon
E -> emp
M -> land mine
D -> detonate land mines

S -> scan
H -> heal

+ -> add
- -> sub
< -> mem prev
> -> mem next
[ -> jump past the matching ] if the current cell value is 0
] -> jump past the matching [ if the current cell value is not 0
( -> start procedure numbered by current cell value
) -> end procedure
: -> run procedure numbered by current cell value
; -> return 

$ -> pocket

    if pocket is empty:
      copy current cell value into pocket
      set current cell value to zero
    else:
      set current cell value to pocket value
      set pocket value to zero

@ -> halt
