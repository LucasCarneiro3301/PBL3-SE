#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include "mlp.h"

// Função de identidade: z = z
float identity(float z) {
	return z;
}

// Função sigmoidal
float sigmoid(float z) {
	return (1.0/(1.0 + expf(-z)));
}

// Função tangente hiperbólica
float tanhyper(float z) {
	return tanhf(z);
}

// Derivada da função identidade
float d_identity(float z) {
	return 1.0;
}

// Derivada da função sigmoidal
float d_sigmoid(float z) {
	return (z*(1.0 - z));
}

// Derivada da função tangente hiperbólica
float d_tanhyper(float z) {
	return 1.0 - z * z;
}

/**
 * @brief Inicializa uma estrutura de rede neural MLP (Perceptron Multicamadas).
 *
 * Esta função aloca dinamicamente e inicializa os pesos das camadas escondida e de saída,
 * bem como os vetores de saída dessas camadas, com valores aleatórios entre aproximadamente -0.5 e 0.5.
 * Também define os parâmetros da MLP, como número de neurônios por camada, taxa de aprendizado,
 * número máximo de épocas e limiar de erro.
 *
 * @param mlp Ponteiro para a estrutura MLP
 * @param input_layer_length Número de neurônios da camada de entrada.
 * @param hidden_layer_length Número de neurônios da camada escondida.
 * @param output_layer_length Número de neurônios da camada de saída.
 * @param max_epochs Número máximo de épocas para o treinamento.
 * @param learning_rate Taxa de aprendizado para ajuste dos pesos.
 * @param threshold Limiar de erro utilizado como critério de parada.
 */
void model(MLP* mlp, int input_layer_length, int hidden_layer_length, int output_layer_length, int max_epochs, float learning_rate, float threshold) {
	mlp->input_layer_length = input_layer_length;
	mlp->hidden_layer_length = hidden_layer_length;
	mlp->output_layer_length = output_layer_length;
	mlp->max_epochs = max_epochs;
	mlp->learning_rate = learning_rate;
	mlp->threshold = threshold;

	srand(time(0));

	mlp->hidden_layer_weights = malloc(hidden_layer_length * sizeof(float*));

	for(int i = 0; i < hidden_layer_length; i++) {
		mlp->hidden_layer_weights[i] = malloc((input_layer_length + 1) * sizeof(float));
		for(int j = 0; j < (input_layer_length + 1); j++) {
			mlp->hidden_layer_weights[i][j] = 2.0f * ((float)rand() / (2.0f * (float)RAND_MAX)) - 0.5f;
		}
	}

	mlp->output_layer_weights = malloc(output_layer_length * sizeof(float*));

	for(int i = 0; i < output_layer_length; i++) {
		mlp->output_layer_weights[i] = malloc((hidden_layer_length + 1) * sizeof(float));
		for(int j = 0; j < (hidden_layer_length + 1); j++) {
			mlp->output_layer_weights[i][j] = 2.0f * ((float)rand() / (2.0f * (float)RAND_MAX)) - 0.5f;
		}
	}

	mlp->hidden_layer_outputs = malloc(hidden_layer_length * sizeof(float));
	mlp->output_layer_outputs = malloc(output_layer_length * sizeof(float));
}

/**
 * @brief Executa a propagação direta da MLP para uma entrada fornecida.
 *
 * Esta função calcula a saída da rede neural para um vetor de entrada X, utilizando os pesos
 * já definidos na estrutura MLP.
 *
 *
 * @param mlp Ponteiro para a estrutura MLP
 * @param X Vetor de entrada
 */
void forward(MLP* mlp, float* X) {
	float hidden_layer_net = 0, output_layer_net = 0;

	for(int i = 0; i < mlp->hidden_layer_length; i++) {
		hidden_layer_net = 0;
		for(int j = 0; j < (mlp->input_layer_length); j++) {
			hidden_layer_net += mlp->hidden_layer_weights[i][j] * X[j];
		}
		hidden_layer_net += mlp->hidden_layer_weights[i][mlp->input_layer_length];
		mlp->hidden_layer_outputs[i] = sigmoid(hidden_layer_net);
	}

	for(int i = 0; i < mlp->output_layer_length ; i++) {
		output_layer_net = 0;
		for(int j = 0; j < mlp->hidden_layer_length; j++) {
			output_layer_net += mlp->output_layer_weights[i][j] * mlp->hidden_layer_outputs[j];
		}
		output_layer_net += mlp->output_layer_weights[i][mlp->hidden_layer_length];
		mlp->output_layer_outputs[i] = identity(output_layer_net);
	}
}

/**
 * @brief Treina a rede MLP usando o algoritmo de retropropagação (backpropagation).
 *
 * Esta função realiza o treinamento supervisionado da rede MLP com base em um conjunto de amostras de entrada `X`
 * e suas respectivas saídas esperadas `Y`. O algoritmo ajusta os pesos das camadas escondida e de saída
 * para minimizar o erro quadrático médio entre a saída da rede e a saída desejada.
 *
 * O processo continua até que o erro médio fique abaixo de um limiar ou que o número máximo de
 * épocas seja atingido.
 *
 * - A função `forward` é usada para calcular a saída atual da rede.
 * - As derivadas `d_identity` e `d_sigmoid` são utilizadas para propagar o erro.
 * - Os pesos são atualizados usando o gradiente descendente.
 *
 * @param mlp Ponteiro para a estrutura MLP que contém pesos, parâmetros e saídas da rede.
 * @param X Matriz de entradas de treinamento (samples x input_layer_length).
 * @param Y Matriz de saídas desejadas para cada entrada (samples x output_layer_length).
 * @param samples Número de amostras de treinamento (linhas de X e Y).
 */
void backpropagation(MLP* mlp, float** X, float** Y, int samples) {
	int epoch = 0;
	float quad_error = 2*mlp->threshold;
	float delta_hidden_layer[mlp->hidden_layer_length], delta_output_layer[mlp->output_layer_length];
	
	while(quad_error > mlp->threshold && epoch < mlp->max_epochs) {
		quad_error = 0;

		for(int i = 0; i < samples; i++) {
			forward(mlp, X[i]);
		
			for(int j = 0; j < mlp->output_layer_length; j++) {
				float error = Y[i][j] - mlp->output_layer_outputs[j];
				
				quad_error += pow(error, 2);

				for(int k  = 0; k < mlp->hidden_layer_length; k++) {
					mlp->output_layer_weights[j][k] -= mlp->learning_rate * - 2 * error * d_identity(mlp->output_layer_outputs[j]) * mlp->hidden_layer_outputs[k];
				}
				mlp->output_layer_weights[j][mlp->hidden_layer_length] -= mlp->learning_rate * - 2 * error * d_identity(mlp->output_layer_outputs[j]) * 1;
			}

			for(int j = 0; j < mlp->hidden_layer_length; j++) {
				float sum = 0;

				for(int k  = 0; k < mlp->output_layer_length; k++) {
					float error = Y[i][k] - mlp->output_layer_outputs[k];

					sum += - 2 * error * d_identity(mlp->output_layer_outputs[k]) * mlp->output_layer_weights[k][j];
				}

				for(int k  = 0; k < mlp->input_layer_length; k++) {
					mlp->hidden_layer_weights[j][k] -= mlp->learning_rate * sum * d_sigmoid(mlp->hidden_layer_outputs[j]) * X[i][k];
				}
				mlp->hidden_layer_weights[j][mlp->input_layer_length] -= mlp->learning_rate * sum * d_sigmoid(mlp->hidden_layer_outputs[j]) ;
			}
		}
		
		quad_error = quad_error/samples;
		epoch++;

        printf("Erro medio: %f\n", quad_error);
	}
}