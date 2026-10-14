# Undertone is an Audio Stegnography Analyzer, Decoder and Massege Embedder

**Applying DSP, Stegnography concepts to build an audio and message processing tool**

## How to Use?
- Analyze (and Decode if found encoded with message)  
- Input an audio file (.wav) and embed it with favoured message 

1. Embed a message into a pristine WAV file
```Bash
./Undertone -e -i clean.wav -m "crypto is fun" -o stego.wav
```

Output: Successfully embedded payload into stego.wav
2. Analyze both files for comparison
```Bash
./Undertone -a -i clean.wav
```
Output:
Analysis Results for clean.wav:
Steganography Probability: 4.20%
Status: CLEAN (Natural audio features)
```Bash
./Undertone -a -i stego.wav
```
Output:
Analysis Results for stego.wav:
Steganography Probability: 91.50%
Status: SUSPICIOUS (High likelihood of LSB tampering)
3. Extract and decode the hidden payload
```Bash
./Undertone -x -i stego.wav
```
Output: Decoded Payload: "crypto is fun"


```codedByJana```