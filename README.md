
No presente sistema, a gestão dos índices de memória é realizada através da utilização de Árvores B+ e Tabelas Hash, cada uma desempenhando um papel específico na organização e acesso eficiente aos dados.

As Árvores B+ empregadas neste sistema são do tipo B+. Especificamente, foram implementadas duas instâncias de Árvores B+: uma para o campo product_id e outra para o campo price. Ambas as árvores possuem um grau de 257, o que significa que cada nó interno pode conter até 256 chaves. 

Para os campos brand e category_code, o sistema utiliza Tabelas Hash, implementadas na classe HashTable. Cada tabela possui um tamanho de 256 buckets, correspondendo ao número de posições disponíveis para armazenar as chaves. A função de hash utilizada soma os valores ASCII de todos os caracteres da chave e aplica o operador módulo com o tamanho da tabela (256) para determinar o índice de armazenamento.

A estratégia adotada para a resolução de colisões nas Tabelas Hash é o encadeamento (chaining). Nesta abordagem, cada bucket da tabela hash mantém uma lista ligada de pares chave-valor. Quando múltiplas chaves resultam no mesmo índice de hash, elas são armazenadas na mesma lista ligada correspondente ao bucket, evitando assim a sobreposição de dados e permitindo a recuperação eficiente de todos os valores associados a uma determinada chave. Adicionalmente, quando há valores repetidos para uma mesma chave, o sistema armazena uma lista de listas dentro do bucket correspondente. 
