# SimulatingShoe
This Repository contains 3D Moddles of an Shoe, that tryes to simulate different undergrounds.

Here is a tabular documentation of the HTTP interface for the different endpoints in English:

| Endpoint                  | Method | Description                                                                                                     |
|---------------------------|--------|-----------------------------------------------------------------------------------------------------------------|
| `/`                       | GET    | Returns the main page of the application (index_html)                                                          |
| `/parameter/:param/value/:value` | GET | Sets parameter `:param` to the value `:value`. See parameter table below for details                          |
| `/vibrate/:id`            | GET    | Vibrates motor `:id` (0-5) for 1 second                                                                        |
| `/stop`                   | GET    | Stops the upload and the valve system                                                                          |
| `/start`                  | GET    | Starts the upload and the valve system                                                                         |
| `/status`                 | GET    | Returns information about the last measurement, last upload, number of elements, and free heap                |
| `/vibrate`                | GET    | Toggles vibration on or off                                                                                     |
| `/asphalt`                | GET    | Simulates asphalt                                                                                               |
| `/grass`                  | GET    | Simulates grass                                                                                                |
| `/sand`                   | GET    | Simulates sand                                                                                                  |
| `/lenolium`               | GET    | Simulates linoleum                                                                                              |
| `/gravel`                 | GET    | Simulates gravel                                                                                                |
| `/evenout`                | GET    | Evens out the valve system                                                                                      |
| `/restart`                | GET    | Restarts the system                                                                                            |

Parameter table:

| Parameter ID | Description                             |
|--------------|-----------------------------------------|
| 0            | Sets the pressure (value * 10)          |
| 1            | Vibrates the motor on the ground        |
| 2            | Vibrates the motor in the air           |
| 3            | Vibrates the motor continuously         |
| 4            | (not used)                              |
| 5            | (not used)                              |

The documentation describes each endpoint and its function. The parameter table provides information about the supported parameters and their meaning.