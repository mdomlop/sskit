mksnpd
------

Carga /etc/sntab en memoria y ejecuta mksnp por cada línea cada minuto. Y
luego ejecuta clsnp para todas las líneas.

La frecuencia 0 significa sin frecuencia. Es decir, únicamente una vez, cuando
se inicia el programa. Está pensado para que se ejecutae al inicio del sistema.

#### Ejemplo de `/etc/sstab`

    # subv    snapdir            frequency    quota
    /         /backup/root/boot  0            30
    /         /backup/root/diary 1d           30
    /home     /backup/home/30m   30m          20

mksnp
-----

Captura el subv en el directorio especificado, si ha pasado el plazo establecido
por la frecuencia. Y sólo si ha habido cambios. El formato es 
'+%Y-%m-%d_%H-%M-%S' (2023-02-24_07-43-20)


clsnp
-----

Elimina los snapshots más viejos hasta cumplir la cuota.


stsnp
-----

Muestra stadísticas de los snapshots

