# ai-parking-nodes

Each parking slot (e.g., A1) has a small embedded node: an Arduino Uno R4 WiFi facing the slot with a Grove AI Vision Module running a SenseCraft-trained “car / no car” model. The Grove module performs all the heavy AI inference at the edge and streams simple serial data (for example, a label like car plus confidence) to the Uno via UART. The Uno parses this serial data, decides if the slot is occupied or empty, and keeps a local state variable for that specific slot ID (e.g., "A1").
