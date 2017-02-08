IDEAS
CURRENTLY NOT USED

int epics_test_fb_general() {
	auto pvstr = epics::nt::NTNDArray::createBuilder()
	->addAlarm()
	->createPVStructure();
	auto const type = epics::pvData::ScalarType::pvFloat;
	// Note how we have to specify the basic scalar type here:
	auto a = epics::pvData::getPVDataCreate()->createPVScalarArray<epics::pvData::PVValueArray<float>>();
	// Fill with dummy data:
	a->setLength(0);
	//int xx = a;
	auto a1 = a->reuse();
	for (auto & x : a1) { x = 0.1; }
	a->replace(epics::pvData::freeze(a1));
	if (auto u = dynamic_cast<epics::pvData::PVUnion*>(pvstr->getSubField("value").get())) {
		auto n = std::string(epics::pvData::ScalarTypeFunc::name(type)) + "Value";
		u->set(n, a);
	}

	{
		// TODO
		// Push the attribute into the actual PV
		auto att1_ = epics::nt::NTAttribute::createBuilder()->create();
		att1_->getName()->put("att1_name");
		auto att1 = att1_->getPVStructure();
		auto x1 = epics::pvData::getPVDataCreate()->createPVScalar(epics::pvData::ScalarType::pvFloat);
		att1_->getValue()->set(x1);
	}

	pvstr->dumpValue(std::cout);
	auto sequence_number = 123;
	auto fb = BrightnESS::ForwardEpicsToKafka::Epics::conv_to_fb_general(BrightnESS::ForwardEpicsToKafka::TopicMappingSettings("ch1", "tp1"), pvstr, sequence_number, 440055, 42);
	if (true) {
		auto b = fb->builder.get();
		fmt::print("builder raw pointer after Finish: {}\n", (void*)b->GetBufferPointer());
		auto p1 = b->GetBufferPointer();
		auto veri = flatbuffers::Verifier(p1, b->GetSize());
		if (not VerifyPVBuffer(veri)) {
			throw std::runtime_error("Bad buffer");
		}
		else {
			LOG(3, "Verified");
		}
	}
	if (true) {
		// Print the test buffer
		auto d1 = fb->message();
		fmt::print("d1 raw ptr: {}\n", (void*)d1.data);
		auto b1 = binary_to_hex((char*)d1.data, d1.size);
		fmt::print("Tested buffer in hex, check for schema tag:\n{:{}}\n", b1.data(), b1.size());
	}
	return 0;
}


#if HAVE_GTEST
#include <gtest/gtest.h>

TEST(ssdfds, sdfdshf) {
	epics_test_fb_general();
}
#endif
