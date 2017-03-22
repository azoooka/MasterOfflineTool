#include "NeuralNetworkFactory.h"
#include "NeuralNetwork.h"
#include <ConfigHandler.h>
#include <FileHandler.h>
#include <sstream>

#include <iostream>
using namespace std;
NeuralNetworkFactory::NeuralNetworkFactory() : m_validationData(nullptr), m_deleteCompletedNetworks(true)
{
}


NeuralNetworkFactory::~NeuralNetworkFactory()
{
    JoinNetworkThreads();
}

void NeuralNetworkFactory::CreateNewNeuralNetworkActivationFunctionCombinationFromData(FANN::training_data * p_trainingData, const int & p_numberOfHiddenLayers,
    int * p_hiddenLayerCells, const float & p_learningRateSteepness,
    const float & p_steepnessOutput, const float & p_steepnessHidden, const bool & p_deteministicWeights,
    const int& p_numberOfEpochsToTrain, const int& p_reportRate, const float& p_accaptableError, FANN::training_data* p_validationData,
    const std::string& p_netIdString)
{
    NetworkSettings newNetSettings;
    // Set the constant variables
    newNetSettings.inputCells = p_trainingData->num_input_train_data();
    newNetSettings.outputCells = p_trainingData->num_output_train_data();
    // This will be a leak...
    newNetSettings.trainingData = new FANN::training_data(*p_trainingData);
    newNetSettings.hiddenCells = p_hiddenLayerCells;
    newNetSettings.hiddenLayers = p_numberOfHiddenLayers;
    newNetSettings.learningRate = p_learningRateSteepness;
    newNetSettings.steepnessHidden = p_steepnessHidden;
    newNetSettings.steepnessOutput = p_steepnessOutput;
    newNetSettings.deterministicWeights = p_deteministicWeights;
    newNetSettings.idString = p_netIdString;
    m_epocsToTrain = p_numberOfEpochsToTrain;
    m_reportRate = p_reportRate;
    m_errorAcceptance = p_accaptableError;
    
    // Se if we have any validation data
    if (p_validationData == nullptr)
    {
        // If m_validationdata is nullptr no validation will take place
        if (m_validationData == nullptr)
        {
            newNetSettings.validationData = nullptr;
        }
        else
            newNetSettings.validationData = new FANN::training_data(*m_validationData);
    }
    else
    {
        newNetSettings.validationData = new FANN::training_data(*p_validationData);
    }
    CreateFANNFunctionOutputSpecificCombinations(&newNetSettings);
}

void NeuralNetworkFactory::CreateNewNeuralNetworkCombinationsFromData(FANN::training_data* p_trainingData, FANN::training_data* p_validationData)
{
    m_networksTrainedWithCurrentData = 0;
    NetworkSettings newNetSettings;
    // Set the constant variables
    newNetSettings.inputCells = p_trainingData->num_input_train_data();
    newNetSettings.outputCells = p_trainingData->num_output_train_data();
    newNetSettings.trainingData = p_trainingData;
    // Se if we have any validation data
    if (p_validationData == nullptr)
    {
        // If m_validationdata is nullptr no validation will take place
        newNetSettings.validationData = m_validationData;
    }
    else
    {
        newNetSettings.validationData = p_validationData;
    }

    // Unroll the first special case where we have no hidden layers
    newNetSettings.hiddenLayers = 0;
    CreateLearningRateSteepnessAndDeterministicSpecificCombinations(&newNetSettings);
    // Now we start looping over the different combinations of things, starting with 
    for (size_t hiddenLayers = 1; hiddenLayers <= m_maxNumberOfHiddenLayers; hiddenLayers++)
    {
        // Save number of layers to the settingstruct
        newNetSettings.hiddenLayers = hiddenLayers;
        // We allocate memory for the hidden cells
        int* hiddenCells = (int*)malloc(hiddenLayers * sizeof(int));
        for (size_t i = 0; i < hiddenLayers; i++)
        {
            hiddenCells[i] = 1;
        }

        // Here we unroll one of the loops that is in the recursive method
        // This is done because we want to use the hidden cell count without it having been incremented once
        for (size_t i = 0; i < hiddenLayers; i++)
        {
            std::cout << hiddenCells[i];
        }
        std::cout << std::endl;

        newNetSettings.hiddenLayers = hiddenLayers;
        newNetSettings.hiddenCells = hiddenCells;
        // With this combo we create even more combos from other settings
        CreateLearningRateSteepnessAndDeterministicSpecificCombinations(&newNetSettings);

        CreateHiddenLayerCombinations(&newNetSettings, hiddenCells, hiddenLayers, 0);
        std::cout << std::endl;
    }
}

void NeuralNetworkFactory::CreateSpecificNeuralNetwork(const NetworkSettings& p_setting)
{
    NetworkSettings newNetSettings;
    newNetSettings = p_setting;
    LaunchNewNet(&newNetSettings, m_epocsToTrain, m_reportRate, m_errorAcceptance);
    return;
}


void NeuralNetworkFactory::CreateAndRunNetworksFromBaseline(NetworkSettings p_baseline, FANNSettingToTest p_whatToTest)
{
    NetworkSettings t_currentSettings = p_baseline;
    vector<string> t_networks;

    if (((int)p_whatToTest & (int)FANNSettingToTest::TestHiddenCells) == (int)FANNSettingToTest::TestHiddenCells)
    {
        // Hidden cells
        int t_maxHiddenCells = 100;
        int t_hiddenCellsIncrement = 10;
        for (size_t i = t_hiddenCellsIncrement; i <= t_maxHiddenCells; i += t_hiddenCellsIncrement)
        {
            for (size_t j = 0; j < t_currentSettings.hiddenLayers; j++)
            {
                t_currentSettings.hiddenCells[j] = i;
            }
            LaunchNewNet(&t_currentSettings, m_epocsToTrain, m_reportRate, m_errorAcceptance);
            //SetupAndTrainNetworkAndAddResultsToList(&t_networks, t_currentSettings);
        }
        JoinNetworkThreads();
        SaveBestNetworksToString(t_networks);
        FileHandler::AppendToFile(t_networks, "../GraphNetworks/HiddenCells.settings");
        t_networks.clear();
        ClearBestVectors();
    }  

    // Hidden layers
    if (((int)p_whatToTest & (int)FANNSettingToTest::TestHiddenLayers) == (int)FANNSettingToTest::TestHiddenLayers)
    {
        // Potential memory leak?
        t_currentSettings = p_baseline;
        int t_maxLayers = 5;
        for (size_t i = 0; i < t_maxLayers; i++)
        {
            t_currentSettings.hiddenLayers = i;
            for (size_t j = 0; j < t_currentSettings.hiddenLayers; j++)
            {
                if (p_baseline.hiddenLayers > 0)
                {
                    t_currentSettings.hiddenCells[j] = p_baseline.hiddenCells[0];
                }
                else
                {
                    t_currentSettings.hiddenCells[j] = 10; // This shouldn't happen though. Ever
                }
            }
            LaunchNewNet(&t_currentSettings, m_epocsToTrain, m_reportRate, m_errorAcceptance);
            //SetupAndTrainNetworkAndAddResultsToList(&t_networks, t_currentSettings);
        }
        JoinNetworkThreads();
        SaveBestNetworksToString(t_networks);
        FileHandler::AppendToFile(t_networks, "../GraphNetworks/HiddenLayers.settings");
        t_networks.clear();
        ClearBestVectors();
    }

    // Learning rate
    if (((int)p_whatToTest & (int)FANNSettingToTest::TestLearningRate) == (int)FANNSettingToTest::TestLearningRate)
    {
        t_currentSettings = p_baseline;
        float t_learningRateIncrement = 0.1;
        for (float i = 0; i <= 1; i += t_learningRateIncrement)
        {
            t_currentSettings.learningRate = i;
            LaunchNewNet(&t_currentSettings, m_epocsToTrain, m_reportRate, m_errorAcceptance);
            // SetupAndTrainNetworkAndAddResultsToList(&t_networks, t_currentSettings);
        }
        JoinNetworkThreads();
        SaveBestNetworksToString(t_networks);
        FileHandler::AppendToFile(t_networks, "../GraphNetworks/LearningRate.settings");
        t_networks.clear();
        ClearBestVectors();
    }

    // Steepness hidden
    if (((int)p_whatToTest & (int)FANNSettingToTest::TestHiddenSteepness) == (int)FANNSettingToTest::TestHiddenSteepness)
    {
        t_currentSettings = p_baseline;
        float t_steepnessHiddenIncrement = 0.1;
        for (float i = 0; i <= 1; i += t_steepnessHiddenIncrement)
        {
            t_currentSettings.steepnessHidden = i;
            LaunchNewNet(&t_currentSettings, m_epocsToTrain, m_reportRate, m_errorAcceptance);
            //SetupAndTrainNetworkAndAddResultsToList(&t_networks, t_currentSettings);
        }
        JoinNetworkThreads();
        SaveBestNetworksToString(t_networks);
        FileHandler::AppendToFile(t_networks, "../GraphNetworks/SteepnessHidden.settings");
        t_networks.clear();
        ClearBestVectors();
    }

    // Steepness output
    if (((int)p_whatToTest & (int)FANNSettingToTest::TestOutputSteepness) == (int)FANNSettingToTest::TestOutputSteepness)
    {
        t_currentSettings = p_baseline;
        float t_steepnessOutputIncrement = 0.1;
        for (float i = 0; i <= 1; i += t_steepnessOutputIncrement)
        {
            t_currentSettings.steepnessOutput = i;
            LaunchNewNet(&t_currentSettings, m_epocsToTrain, m_reportRate, m_errorAcceptance);
            // SetupAndTrainNetworkAndAddResultsToList(&t_networks, t_currentSettings);
        }
        JoinNetworkThreads();
        SaveBestNetworksToString(t_networks);
        FileHandler::AppendToFile(t_networks, "../GraphNetworks/SteepnessOutput.settings");
        t_networks.clear();
        ClearBestVectors();
    }

    //  Hidden Function
    if (((int)p_whatToTest & (int)FANNSettingToTest::TestHiddenLayerFunction) == (int)FANNSettingToTest::TestHiddenLayerFunction)
    {
        t_currentSettings = p_baseline;
        // Loop over all different functions, COS_SYMMETRIC is the last in the enum
        for (size_t i = 0; i < FANN::activation_function_enum::COS_SYMMETRIC + 1; i++)
        {
            if (i == FANN::activation_function_enum::THRESHOLD || i == FANN::activation_function_enum::THRESHOLD_SYMMETRIC)
            {
                continue;
            }
            t_currentSettings.functionHidden = static_cast<FANN::activation_function_enum>(i);
            LaunchNewNet(&t_currentSettings, m_epocsToTrain, m_reportRate, m_errorAcceptance);
        }
        JoinNetworkThreads();
        SaveBestNetworksToString(t_networks);
        FileHandler::AppendToFile(t_networks, "../GraphNetworks/HiddenFunction.settings");
        t_networks.clear();
        ClearBestVectors();
    }

    //  output Function
    if (((int)p_whatToTest & (int)FANNSettingToTest::TestOutputLayerFunction) == (int)FANNSettingToTest::TestOutputLayerFunction)
    {
        t_currentSettings = p_baseline;
        // Loop over all different functions, COS_SYMMETRIC is the last in the enum
        for (size_t i = 0; i < FANN::activation_function_enum::COS_SYMMETRIC + 1; i++)
        {
            if (i == FANN::activation_function_enum::THRESHOLD || i == FANN::activation_function_enum::THRESHOLD_SYMMETRIC)
            {
                continue;
            }
            t_currentSettings.functionOutput = static_cast<FANN::activation_function_enum>(i);
            LaunchNewNet(&t_currentSettings, m_epocsToTrain, m_reportRate, m_errorAcceptance);
        }
        JoinNetworkThreads();
        SaveBestNetworksToString(t_networks);
        FileHandler::AppendToFile(t_networks, "../GraphNetworks/OutputFunction.settings");
        t_networks.clear();
        ClearBestVectors();
    }

    //  training algorithm
    if (((int)p_whatToTest & (int)FANNSettingToTest::TestTrainingAlgorithm) == (int)FANNSettingToTest::TestTrainingAlgorithm)
    {
        t_currentSettings = p_baseline;
        // Loop over all different functions, TRAIN_SARPROP is the last in the enum
        for (size_t i = 0; i < FANN::training_algorithm_enum::TRAIN_SARPROP + 1; i++)
        {
            t_currentSettings.trainingAlgorithm = static_cast<FANN::training_algorithm_enum>(i);
            LaunchNewNet(&t_currentSettings, m_epocsToTrain, m_reportRate, m_errorAcceptance);
        }
        JoinNetworkThreads();
        SaveBestNetworksToString(t_networks);
        FileHandler::AppendToFile(t_networks, "../GraphNetworks/TrainAlgorithm.settings");
        t_networks.clear();
        ClearBestVectors();
    }
}

void NeuralNetworkFactory::SetupAndTrainNetworkAndAddResultsToList(std::vector<std::string>* p_netResults, const NetworkSettings& p_netSettings)
{
    NeuralNetwork t_net;
    t_net.SetSettings(p_netSettings);
    t_net.SetupNetwork();
    t_net.TrainAndValidateNetwork(m_epocsToTrain, m_reportRate, m_errorAcceptance);
    p_netResults->push_back(FileHandler::SaveNetworkToString(t_net.GetNetworkSettings()));
}

void NeuralNetworkFactory::CreateHiddenLayerCombinations(NetworkSettings * p_netWorkSettings, int* p_hiddenCells, const int& p_numberOfLayers, const int& p_depth)
{
    while (true)
    {
        if (p_depth + 1 != p_numberOfLayers)
        {
            CreateHiddenLayerCombinations(p_netWorkSettings, p_hiddenCells, p_numberOfLayers, p_depth + 1);
        }
        if (p_hiddenCells[p_depth] >= m_maxNumberOfHiddenCellsPerLayer)
        {
            p_hiddenCells[p_depth] = 1; // Might need to be 1, dont know if FANN accepts 0 as in
            return;
        }

        p_hiddenCells[p_depth] += m_numberOfHiddenCellIncrement;
        for (size_t i = 0; i < p_numberOfLayers; i++)
        {
            std::cout << p_hiddenCells[i];
        }
        std::cout << std::endl;
        // Now we put in the info just created into the info and pass it to the next chain in the functions 
        // to create more combinations that builds on this combo
        p_netWorkSettings->hiddenLayers = p_numberOfLayers;
        p_netWorkSettings->hiddenCells = p_hiddenCells;
        // With this combo we create even more combos from other settings
        CreateLearningRateSteepnessAndDeterministicSpecificCombinations(p_netWorkSettings);
    }
}

void NeuralNetworkFactory::CreateLearningRateSteepnessAndDeterministicSpecificCombinations(NetworkSettings * p_netWorkSettings)
{
    for (float learningRate = m_learningRateIncrement; learningRate <= 1; learningRate += m_learningRateIncrement)
    {
        p_netWorkSettings->learningRate = learningRate;
        for (float hiddenSteepness = m_hiddenSteepnessIncrement; hiddenSteepness <= 1; hiddenSteepness += m_hiddenSteepnessIncrement)
        {
            p_netWorkSettings->steepnessHidden = hiddenSteepness;
            for (float outputSteepness = m_outputSteepnessIncrement; outputSteepness <= 1; outputSteepness += m_outputSteepnessIncrement)
            {
                p_netWorkSettings->steepnessOutput = outputSteepness;
                for (size_t i = 0; i < 2; i++)
                {
                    // This should give 0/1 = 0 and 1/1 = 1 aka false and true
                    p_netWorkSettings->deterministicWeights = i / 2;
                    CreateFANNFunctionOutputSpecificCombinations(p_netWorkSettings);
                }
            }
        }
    }

}

void NeuralNetworkFactory::CreateFANNFunctionOutputSpecificCombinations(NetworkSettings * p_networkSettings)
{
    // Loop over all different functions, COS_SYMMETRIC is the last in the enum
    for (size_t i = 0; i < FANN::activation_function_enum::COS_SYMMETRIC + 1; i++)
    {
        if (i == FANN::activation_function_enum::THRESHOLD || i == FANN::activation_function_enum::THRESHOLD_SYMMETRIC)
        {
            continue;
        }
        p_networkSettings->functionOutput = static_cast<FANN::activation_function_enum>(i);
        CreateFANNFunctionHiddenSpecificCombinations(p_networkSettings);
    }
}

void NeuralNetworkFactory::CreateFANNFunctionHiddenSpecificCombinations(NetworkSettings * p_networkSettings)
{
    // Loop over all different functions, COS_SYMMETRIC is the last in the enum
    // The threshold once cant be used during training so for now we skipp them completely (Should maybe use them during validation?)
    for (size_t i = 0; i < FANN::activation_function_enum::COS_SYMMETRIC + 1; i++)
    {
        if (i == FANN::activation_function_enum::THRESHOLD || i == FANN::activation_function_enum::THRESHOLD_SYMMETRIC)
        {
            continue;
        }
        p_networkSettings->functionHidden = static_cast<FANN::activation_function_enum>(i);
        // This is the last stepp, now we create and save the network!
        CreateTheNetwork(p_networkSettings);
    }
}

void NeuralNetworkFactory::CreateTheNetwork(NetworkSettings * p_netWorkSettings)
{
    // We create a new net and save it after passing the accuiered setting
    LaunchNewNet(p_netWorkSettings, m_epocsToTrain, m_reportRate, m_errorAcceptance);
}

void NeuralNetworkFactory::LaunchNewNet(NetworkSettings * p_netWorkSettings, const int & p_epochs, const int & p_reportRate, const float & p_acceptedError)
{
    ThreadedNetwork* newThreadedNetwork = new ThreadedNetwork();
    NeuralNetwork* newNet = new NeuralNetwork();
    newNet->SetSettings(*p_netWorkSettings);
    newNet->SetupNetwork();
    newThreadedNetwork->net = newNet;
    // There is still place in the active network list
    if (m_networks.size() >= m_maxNetsInMemoryAtOneTime)
    {
        JoinNetworkThreads();
    }
    newThreadedNetwork->thread = std::thread(&NeuralNetwork::TrainAndValidateNetwork, newNet, p_epochs, p_reportRate, p_acceptedError);
    m_networks.push_back(newThreadedNetwork);
}

void NeuralNetworkFactory::ClearBestVectors()
{
    size_t length = m_bestNetworks.size();
    for (size_t i = 0; i < length; i++)
    {
        free(m_bestNetworks.at(i).hiddenCells);
    }
    m_bestNetworks.clear();
}

void NeuralNetworkFactory::JoinNetworkThreads()
{
    for (size_t i = 0; i < m_networks.size(); i++)
    {
        m_networks[i]->thread.join();
        UpdateBestNetworks(m_networks[i]->net->GetNetworkSettings());
        if (m_deleteCompletedNetworks)
        {
            delete m_networks[i]->net;
        }
        else
        {
            m_savedNetworks.push_back(m_networks[i]->net);
        }
        delete m_networks[i]; // This should not delete the thing that ->net is pointing to
    }

    m_networks.clear();   
}

void NeuralNetworkFactory::SetDeleteCompletedNetworks(bool p_delete)
{
    m_deleteCompletedNetworks = p_delete;
    if (p_delete)
    {
        size_t length = m_savedNetworks.size();
        for (size_t i = 0; i < length; i++)
        {
            delete m_savedNetworks[i];
        }
        m_savedNetworks.clear();
    }
}

void NeuralNetworkFactory::UpdateBestNetworks(NetworkSettings p_settings)
{
    // Fill up list of best networks if not already full
    if (m_bestNetworks.size() < m_numBestNetworks)
        m_bestNetworks.push_back(p_settings);
    else
    {
        // Find the worst mse of the best networks
        float t_worst = m_bestNetworks.at(0).mse;
        int t_index = 0;
        for (size_t i = 0; i < m_bestNetworks.size(); i++)
        {
            if (m_bestNetworks.at(i).mse > t_worst)
            {
                t_index = i;
                t_worst = m_bestNetworks.at(i).mse;
            }
        }
        // Check if new network is better than worst
        if (p_settings.mse < t_worst)
            m_bestNetworks.at(t_index) = p_settings;
    }
}

void NeuralNetworkFactory::SaveBestNetworksToString(std::vector<std::string>& o_savedNetVector)
{
    for (size_t i = 0; i < m_bestNetworks.size(); i++)
    {
        o_savedNetVector.push_back(FileHandler::SaveNetworkToString(m_bestNetworks[i]));
    }
}
