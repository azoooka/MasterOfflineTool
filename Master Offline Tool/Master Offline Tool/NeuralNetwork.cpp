#include "NeuralNetwork.h"
#include <iostream>
using namespace std;


int training_callback(FANN::neural_net &net, FANN::training_data &train,
	unsigned int max_epochs, unsigned int epochs_between_reports,
	float desired_error, unsigned int epochs, void *user_data)
{
	cout << "Epochs     " << epochs << ". "
		<< "Current Error: " << net.get_MSE() << endl;
	return 0;
}

NeuralNetwork::NeuralNetwork()
{
}


NeuralNetwork::~NeuralNetwork()
{
}

void NeuralNetwork::ValidateOnFile()
{
	m_data.read_train_from_file(m_trainingDataFilename);
	m_net.test_data(m_data);
	std::cout << "Mean Square Error according to FANN: " << m_net.get_MSE() << endl;
	float fullError = 0;
	float* input = *m_data.get_input();
	float* output = *m_data.get_output();
	int length = m_data.length_train_data();
	for (size_t i = 0; i < length; i++)
	{
		float* netOutput = m_net.run(&input[i * 3]);
		std::cout << "Net: " << *netOutput << " Acctual: " << output[i] << endl;
		fullError += abs(abs(*netOutput) - abs(output[i]));
	}
	std::cout << "mean error: " << fullError / static_cast<float>(length) << endl;
}

void NeuralNetwork::TrainOnData(const int& p_numberOfEpochs, const int& p_reportFrequency, const float& p_errorAccaptance)
{
	m_data.read_train_from_file(m_trainingDataFilename);
	m_net.train_on_data(m_data, p_numberOfEpochs, p_reportFrequency, p_errorAccaptance);
}

void NeuralNetwork::SetupNetwork()
{
	// Cases for how many layers we want. Yup, this is silly...
	switch (m_networkSettings.hiddenLayers)
	{
	case 0:
		m_net.create_standard(2,
			m_networkSettings.inputCells,
			m_networkSettings.outputCells);
		break;
	case 1:
		m_net.create_standard(3,
			m_networkSettings.inputCells, 
			m_networkSettings.hiddenCells[0], 
			m_networkSettings.outputCells);
		break;
	case 2:
		m_net.create_standard(4,
			m_networkSettings.inputCells,
			m_networkSettings.hiddenCells[0],
			m_networkSettings.hiddenCells[1],
			m_networkSettings.outputCells);
		break;
	case 3:
		m_net.create_standard(5,
			m_networkSettings.inputCells,
			m_networkSettings.hiddenCells[0],
			m_networkSettings.hiddenCells[1],
			m_networkSettings.hiddenCells[2],
			m_networkSettings.outputCells);
		break;
	}
	// Set learning rate, activation function and steepness
	m_net.set_learning_rate(m_networkSettings.learningRate);
	m_net.set_activation_steepness_output(m_networkSettings.steepnessOutput);
	m_net.set_activation_function_output(m_networkSettings.functionOutput);
	// Hidden settings possibly should be inside an if-condition?
	m_net.set_activation_steepness_hidden(m_networkSettings.steepnessHidden);
	m_net.set_activation_function_hidden(m_networkSettings.functionHidden);

	m_net.set_callback(training_callback, NULL);

}
