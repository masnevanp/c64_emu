;---------------------------------------;ICR01.ASM - THIS FILE IS PART;OF THE �64 �MULATOR �EST �UITE;PUBLIC DOMAIN, NO COPYRIGHT           *= $0801           .BYTE $4C,$14,$08,$00,$97TURBOASS   = 780           .TEXT "780"           .BYTE $2C,$30,$3A,$9E,$32,$30           .BYTE $37,$33,$00,$00,$00           .BLOCK           LDA #1           STA TURBOASS           LDX #0           STX $D3           LDA THISNAMEPRINTTHIS           JSR $FFD2           INX           LDA THISNAME,X           BNE PRINTTHIS           JSR MAIN           LDA #$37           STA 1           LDA #$2F           STA 0           JSR $FD15           JSR $FDA3           JSR PRINT           .TEXT " - OK"           .BYTE 13,0           LDA TURBOASS           BEQ LOADNEXT           JSR WAITKEY           JMP $8000           .BENDLOADNEXT           .BLOCK           LDX #$F8           TXS           LDA NEXTNAME           CMP #"-"           BNE NOTEMPTY           JMP $A474NOTEMPTY           LDX #0PRINTNEXT           JSR $FFD2           INX           LDA NEXTNAME,X           BNE PRINTNEXT           LDA #0           STA $0A           STA $B9           STX $B7           LDA #<NEXTNAME           STA $BB           LDA #>NEXTNAME           STA $BC           JMP $E16F           .BEND;---------------------------------------;PRINT TEXT WHICH IMMEDIATELY FOLLOWS;THE ��� AND RETURN TO ADDRESS AFTER 0PRINT           .BLOCK           PLA           STA NEXT+1           PLA           STA NEXT+2           LDX #1NEXT           LDA $1111,X           BEQ END           JSR $FFD2           INX           BNE NEXTEND           SEC           TXA           ADC NEXT+1           STA RETURN+1           LDA #0           ADC NEXT+2           STA RETURN+2RETURN           JMP $1111           .BEND;---------------------------------------;PRINT HEX BYTEPRINTHB           .BLOCK           PHA           LSR A           LSR A           LSR A           LSR A           JSR PRINTHN           PLA           AND #$0FPRINTHN           ORA #$30           CMP #$3A           BCC NOLETTER           ADC #6NOLETTER           JMP $FFD2           .BEND;---------------------------------------;WAIT UNTIL RASTER LINE IS IN BORDER;TO PREVENT GETTING DISTURBED BY ���SWAITBORDER           .BLOCK           LDA $D011           BMI OKWAIT           LDA $D012           CMP #30           BCS WAITOK           RTS           .BEND;---------------------------------------;WAIT FOR A KEY AND CHECK FOR ����WAITKEY           .BLOCK           JSR $FD15           JSR $FDA3           CLIWAIT           JSR $FFE4           BEQ WAIT           CMP #3           BEQ STOP           RTSSTOP           LDA TURBOASS           BEQ LOAD           JMP $8000LOAD           JSR PRINT           .BYTE 13           .TEXT "BREAK"           .BYTE 13,0           JMP LOADNEXT           .BEND;---------------------------------------THISNAME   .NULL "ICR01"NEXTNAME   .NULL "IMR";---------------------------------------NMIADR     .WORD 0ONNMI           PHA           TXA           PHA           TSX           LDA $0104,X           STA NMIADR+0           LDA $0105,X           STA NMIADR+1           PLA           TAX           PLA           RTIMAIN;---------------------------------------;READ ICR WHEN IT IS $01 AND CHECK IF;$81 FOLLOWS           .BLOCK           SEI           LDA #0           STA $DC0E           STA $DC0F           LDA #$7F           STA $DC0D           LDA #$81           STA $DC0D           BIT $DC0D           LDA #2           STA $DC04           LDA #0           STA $DC05           JSR WAITBORDER           LDA #%00001001           STA $DC0E           LDA $DC0D           LDX $DC0D           CMP #$01           BEQ OK1           JSR PRINT           .BYTE 13           .TEXT "CIA1 ICR IS NOT $01"           .BYTE 0           JSR WAITKEYOK1           CPX #$00           BEQ OK2           JSR PRINT           .BYTE 13           .TEXT "READING ICR=01 DID "           .TEXT "NOT CLEAR INT"           .BYTE 0OK2           .BEND;---------------------------------------;READ ICR WHEN IT IS $01 AND CHECK IF;NMI FOLLOWS           .BLOCK           SEI           LDA #0           STA NMIADR+0           STA NMIADR+1           STA $DD0E           STA $DD0F           LDA #$7F           STA $DD0D           LDA #$81           STA $DD0D           BIT $DD0D           LDA #<ONNMI           STA $0318           LDA #>ONNMI           STA $0319           LDA #2           STA $DD04           LDA #0           STA $DD05           JSR WAITBORDER           LDA #%00001001           STA $DD0E           LDA $DD0D           LDX $DD0D           CMP #$01           BEQ OK1           JSR PRINT           .BYTE 13           .TEXT "CIA2 ICR IS NOT $01"           .BYTE 0           JSR WAITKEYOK1           CPX #$00           BEQ OK2           JSR PRINT           .BYTE 13           .TEXT "READING ICR=01 DID "           .TEXT "NOT CLEAR ICR"           .BYTE 0           JSR WAITKEYOK2           LDA NMIADR+1           BEQ OK3           JSR PRINT           .BYTE 13           .TEXT "READING ICR=01 DID "           .TEXT "NOT PREVENT NMI"           .BYTE 0           JSR WAITKEYOK3           .BEND;---------------------------------------;READ ICR WHEN IT IS $81 AND CHECK IF;NMI FOLLOWS           .BLOCK           SEI           LDA #0           STA NMIADR+0           STA NMIADR+1           STA $DD0E           STA $DD0F           LDA #$7F           STA $DD0D           LDA #$81           STA $DD0D           BIT $DD0D           LDA #<ONNMI           STA $0318           LDA #>ONNMI           STA $0319           LDA #1           STA $DD04           LDA #0           STA $DD05           JSR WAITBORDER           LDA #%00001001           STA $DD0E           LDA $DD0D           LDX $DD0DNMI           CMP #$81           BEQ OK1           JSR PRINT           .BYTE 13           .TEXT "CIA2 ICR IS NOT $81"           .BYTE 0           JSR WAITKEYOK1           CPX #$00           BEQ OK2           JSR PRINT           .BYTE 13           .TEXT "READING ICR=81 DID "           .TEXT "NOT CLEAR ICR"           .BYTE 0           JSR WAITKEYOK2           LDA NMIADR+1           BNE OK3           JSR PRINT           .BYTE 13           .TEXT "READING ICR=81 MUST "           .TEXT "PASS NMI"           .BYTE 0           JSR WAITKEYOK3           .BEND;---------------------------------------;READ ICR WHEN IT IS $00 AND CHECK IF;NMI FOLLOWS           .BLOCK           SEI           LDA #0           STA NMIADR+0           STA NMIADR+1           STA $DD0E           STA $DD0F           LDA #$7F           STA $DD0D           LDA #$81           STA $DD0D           BIT $DD0D           LDA #<ONNMI           STA $0318           LDA #>ONNMI           STA $0319           LDA #3           STA $DD04           LDA #0           STA $DD05           JSR WAITBORDER           LDA #%00001001           STA $DD0E           LDA $DD0D           LDX $DD0DNMI           CMP #$00           BEQ OK1           JSR PRINT           .BYTE 13           .TEXT "CIA2 ICR IS NOT $00"           .BYTE 0           JSR WAITKEYOK1           CPX #$81           BEQ OK2           JSR PRINT           .BYTE 13           .TEXT "READING ICR=00 MAY "           .TEXT "NOT CLEAR ICR"           .BYTE 0           JSR WAITKEYOK2           LDA NMIADR+1           BNE OK3           JSR PRINT           .BYTE 13           .TEXT "READING ICR=00 MAY "           .TEXT "NOT PREVENT NMI"           .BYTE 0           JSR WAITKEYOK3           .BEND;---------------------------------------           RTS