

Si se envian mas de SIZE_PAYLOAD -1 el buffer de la uart sufre un overflow lo que descarta todos los caracteres 
recividos hasta ese momento.




<div style="page-break-before: always;"></div>
## Diagrama de Secuencia de Tareas 

```mermaid
sequenceDiagram
    autonumber
    participant Main as app_main
    participant STask as serialTask (Antena)
    participant Queue as xQueueMsg
    participant TTask as TAGTask
    participant Timer as oneshot_timer
    participant FTask as TAGFileTask
    participant HW as Hardware / SD / Output

    Note over Main: Inicialización de sistema,<br/>logs y cola xQueueMsg

    Main->>STask: xTaskCreate(serialTask, Prio 2)
    Main->>TTask: xTaskCreate(TAGTask, Prio 1)
    Main->>FTask: xTaskCreate(TAGFileTask, Prio 1)

    Note over FTask, HW: Inicialización y monitoreo de SD Card
    FTask->>HW: TAGInit() (Inicializa SD y lee TAG_FILE)
    
    loop Bucle principal serialTask
        STask->>HW: uart_read_bytes(UART_ANTENA)
        STask-->>HW: Bridge UART_ANTENA <-> UART_BRIDGE
        alt Trama recibida termina en '\n'
            STask->>STask: validarFormatoTrama() / Checksum
            alt Trama Válida & DI_1_STATE Activo
                STask->>Queue: xQueueSend(xQueueMsg, &txPacket, 50ms)
            else Trama Inválida o DI_1 Inactivo
                STask--x Queue: Descarta trama
            end
        end
    end

    loop Bucle principal TAGTask
        Queue->>TTask: xQueueReceive(xQueueMsg, &rxPacket, portMAX_DELAY)
        TTask->>TTask: buscarTAG(rxPacket.datos)
        
        alt TAG Válido
            TTask->>HW: Incrementar TRAMBUS_COUNTER
            alt Timer activo
                TTask->>TTask: contador_TAGS_validos++
            else Timer inactivo
                TTask->>HW: Activar DO_1_STATE & TRAMBUS_DETECTADO
                TTask->>Timer: esp_timer_start_once(3000000 us)
            end
        else TAG Inválido
            Note right of TTask: Log "TAG TRAMBUS INVALIDO"
        end
    end

    Note over Timer, HW: Callback de vencimiento de timer (3s)
    Timer->>HW: Decrementa contador. Si llega a 0 -> Apaga DO_1
```


<div style="page-break-before: always;"></div>
## Menu de usuario

```mermaid

graph TD
    ID0["[ID 0] Root: MENU PRINCIPAL<br/>(Tecla '0') [Input: NO]"]
    
    ID1["[ID 1] MENU DEBUG<br/>(Tecla '1') [Input: NO]"]
    ID2["[ID 2] Fozar salidas<br/>(Tecla '2') [Input: NO]"]
    
    ID12["[ID 12] INFO LEVEL 1<br/>(Tecla '1') [Input: SÍ (Y/N)]"]
    ID13["[ID 13] dataStruct<br/>(Tecla '2') [Input: SÍ (Y/N)]"]
    
    ID21["[ID 21] Fozar salidas<br/>(Tecla '1') [Input: SÍ (< index,state >o N)]"]

    ID0 -->|Tecla '1'| ID1
    ID0 -->|Tecla '2'| ID2
    
    ID1 -->|Tecla '1'| ID12
    ID1 -->|Tecla '2'| ID13
    
    ID2 -->|Tecla '1'| ID21 

```

```mermaid
sequenceDiagram
    autonumber
    actor User as Usuario (Terminal Serie)
    participant UI as userInterfaceUpdate()
    participant Engine as Menu Engine (menuUpdate)
    participant Handlers as Node Callbacks

    %% Ciclo de entrada
    User->>UI: Envía carácter (charReceived)
    
    alt charReceived es Tecla de Regreso (GO_BACK)
        UI->>Engine: menuUpdate(GO_BACK)
        Engine-->>UI: Retorna nodo padre
        UI->>UI: Refresca Pantalla (clearScreen & printNode)
        UI->>Handlers: onEnterNode(currentNode)
        UI->>UI: Actualiza estado (aceptandoDatos = nodeRequiresInput)

    else charReceived es Enter ('\n') y aceptandoDatos == true
        UI->>UI: Finaliza string en buffer ('\0')
        UI->>Handlers: procesarDatos(buffer)
        UI->>UI: Limpia buffer
        UI->>UI: aceptandoDatos = false

    else charReceived es Tecla de Navegación (Ej: '1', '2') y aceptandoDatos == false
        UI->>Engine: menuUpdate(charReceived)
        Engine-->>UI: Retorna nodo destino

        opt Cambió de Nodo (currentNode != lastNode)
            UI->>UI: Refresca Pantalla (clearScreen & printNode)
            UI->>Handlers: onEnterNode(currentNode)
            UI->>UI: Actualiza estado (aceptandoDatos = nodeRequiresInput)
        end

    else charReceived es Texto/Dato y aceptandoDatos == true
        UI->>UI: Acumula carácter en el buffer de entrada
    end

    %% Ciclo de actualización en tiempo real (Ejecutado periódicamente)
    rect rgb(240, 240, 240)
        note over UI, Handlers: Refresco continuo de pantalla
        UI->>Handlers: onUpdateNode / autoUpdate(currentNode)
    end
```