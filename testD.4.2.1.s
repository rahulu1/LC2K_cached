        lw      0   4   four
        lw      0   5   four
        beq     0   5   5
        lw      0   6   eight
        add     5   5   5
        add     5   5   5
        beq     0   5   5
        add     5   5   5
        add     5   5   5
        lw      0   4   four
        lw      0   4   four
        lw      0   4   four
        beq     0   4   5
        nor     4   4   4
        lw      0   4   four
        beq     5   6   1
        beq     5   6   3
        add     5   5   5
        halt
        noop
        halt
four    .fill   4
eight   .fill   8
