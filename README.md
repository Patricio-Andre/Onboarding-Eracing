# Onboarding-Eracing
Esse documento é uma forma de registrar o trabalho que realizei como onboarding da e-racing unicamp em 2023.  
A ideia é criar um circuito para um photogate, um portão de luz capaz de registrar a passagem de um objeto, no nosso caso um carro, por ele.  
O circuito integrou o uso de um Node MCU 8266 com um MicroSD Module e um sensor de distância a láser (VL53L0x).  
Confesso que na prática não consegui aplicar o uso do microSD pela falta de experiência em usar essas bibliotecas, 
mas mesmo assim o código funciona perfeitamente, caso não tenha um microSD para registrar o log. 
Nesse caso, o log fica registrado no webserver gerado pelo Node MCU.  
### Problemas e como contorná-los  
Um dos problemas que envolvem o uso do photogate é a necessidade de calibração dele.  
Para contornar esse problema, usei o sensor de distância à laser, que detecta distâncias de até 2.2m.
Para as provas FSAE a largura mínima da pista gira em torno de 4 metros.  
Devido ao comprimento do carro, considerei que a distância do sensor até o carro seja suficiente para detectá-lo.  
O sol também gera um problema de interferência nos sinais captados, usei um tratamento do blog mundo projetado para lidar com isso.  
