        lw      0   5   neg1
        beq     0   5   end
        lw      0   1   data
        add     1   1   2
        add     1   5   1
        nor     0   2   3
        sw      0   1   data
        halt
