# SimulatingShoe
This Repository contains 3D Moddles of an Shoe, that tryes to simulate different undergrounds.

Hier ist eine tabellarische Dokumentation der HTTP-Schnittstelle für die verschiedenen Endpunkte:

| Endpunkt                  | Methode | Beschreibung                                                                                                     |
|---------------------------|---------|------------------------------------------------------------------------------------------------------------------|
| `/`                       | GET     | Liefert die Hauptseite der Anwendung (index_html)                                                               |
| `/parameter/:param/value/:value` | GET     | Setzt Parameter `:param` auf den Wert `:value`. Siehe Parameter-Tabelle unten für Details                        |
| `/vibrate/:id`            | GET     | Lässt Motor `:id` (0-5) für 1 Sekunde vibrieren                                                                  |
| `/stop`                   | GET     | Stoppt den Upload und das Ventilsystem                                                                           |
| `/start`                  | GET     | Startet den Upload und das Ventilsystem                                                                          |
| `/status`                 | GET     | Liefert Informationen über die letzte Messung, den letzten Upload, die Anzahl der Elemente und den freien Heap |
| `/vibrate`                | GET     | Schaltet Vibration ein oder aus                                                                                   |
| `/asphalt`                | GET     | Simuliert Asphalt                                                                                                |
| `/grass`                  | GET     | Simuliert Gras                                                                                                   |
| `/sand`                   | GET     | Simuliert Sand                                                                                                   |
| `/lenolium`               | GET     | Simuliert Linoleum                                                                                               |
| `/gravel`                 | GET     | Simuliert Kies                                                                                                   |
| `/evenout`                | GET     | Glättet das Ventilsystem                                                                                         |
| `/restart`                | GET     | Startet das System neu                                                                                           |

Parameter-Tabelle:

| Parameter-ID | Beschreibung                             |
|--------------|------------------------------------------|
| 0            | Setzt den Druck (Wert * 10)              |
| 1            | Lässt den Motor auf dem Boden vibrieren  |
| 2            | Lässt den Motor in der Luft vibrieren    |
| 3            | Lässt den Motor kontinuierlich vibrieren |
| 4            | (nicht belegt)                           |
| 5            | (nicht belegt)                           |

Die Dokumentation beschreibt jeden Endpunkt und seine Funktion. Die Parameter-Tabelle gibt Informationen über die unterstützten Parameter und ihre Bedeutung.